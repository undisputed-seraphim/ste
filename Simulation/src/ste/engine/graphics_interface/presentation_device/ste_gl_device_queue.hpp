//	StE
// � Shlomi Steinberg 2015-2017

#pragma once

#include <stdafx.hpp>
#include <ste_gl_device_queues_protocol.hpp>

#include <vk_queue.hpp>
#include <vk_logical_device.hpp>
#include <vk_command_pool.hpp>
#include <vk_command_buffers.hpp>
#include <vk_fence.hpp>

#include <concurrent_queue.hpp>

#include <mutex>
#include <condition_variable>
#include <functional>
#include <interruptible_thread.hpp>
#include <thread_pool_task.hpp>

namespace StE {
namespace GL {

class ste_gl_device_queue {
public:
	using task_t = unique_thread_pool_type_erased_task<const std::uint32_t&>;
	template <typename R>
	using enqueue_task_t = unique_thread_pool_task<R, const std::uint32_t&>;

private:
	using task_pair_t = std::pair<task_t, std::uint32_t>;

private:
	vk_queue queue;
	ste_gl_queue_descriptor descriptor;
	vk_command_pool pool;
	vk_command_buffers buffers;
	std::vector<vk_fence> fences;

	mutable std::mutex m;
	std::condition_variable notifier;
	std::unique_ptr<interruptible_thread> thread;

	concurrent_queue<task_pair_t> task_queue;

private:
	static thread_local vk_command_buffers *command_buffers;

public:
	static auto& thread_command_buffers() { return *command_buffers; }

public:
	ste_gl_device_queue(const vk_logical_device &device, 
						ste_gl_queue_descriptor descriptor,
						std::uint32_t buffers_count)
		: queue(device, descriptor.family, 0),
		descriptor(descriptor), 
		pool(device, descriptor.family),
		buffers(pool.allocate_buffers(buffers_count))
	{
		// Create the fences in signaled state
		for (int i = 0; i < buffers_count; ++i)
			fences.emplace_back(device, true);

		// Create the queue worker thread
		thread = std::make_unique<interruptible_thread>([this]() {
			// Set the thread_local global to this thread's parameters
			ste_gl_device_queue::command_buffers = &this->buffers;

			for (;;) {
				if (interruptible_thread::is_interruption_flag_set()) return;

				std::unique_ptr<task_pair_t> task;
				{
					std::unique_lock<std::mutex> l(this->m);
					if (interruptible_thread::is_interruption_flag_set()) return;

					this->notifier.wait(l, [&]() {
						return interruptible_thread::is_interruption_flag_set() ||
							(task = task_queue.pop()) != nullptr;
					});
				}

				while (task != nullptr) {
					if (interruptible_thread::is_interruption_flag_set())
						return;

					// Call task lambda, passing command buffer index
					task->first(task->second);
					task = task_queue.pop();
				}
			}
		});
	}
	~ste_gl_device_queue() noexcept {
		thread->interrupt();

		do { notifier.notify_all(); } while (!m.try_lock());
		m.unlock();

		thread->join();
	}

	ste_gl_device_queue(ste_gl_device_queue &&q) = delete;
	ste_gl_device_queue &operator=(ste_gl_device_queue &&) = delete;
	ste_gl_device_queue(const ste_gl_device_queue &) = delete;
	ste_gl_device_queue &operator=(const ste_gl_device_queue &) = delete;

	/**
	*	@brief	Enqueues a task on the queue's thread
	*
	*	@param	f					Lambda expression
	*	@param	buffer_index		Index of the command buffer to pass to the task
	*/
	template <typename R>
	std::future<R> enqueue_with_buffer_idx(enqueue_task_t<R> &&f, std::uint32_t buffer_index) {
		auto future = f.get_future();

		task_queue.push(std::make_pair(std::move(f), buffer_index));
		notifier.notify_one();

		return future;
	}

	/**
	*	@brief	Resets the command buffer
	*			Might stall if command buffer is still executing on the device
	*
	*	@param	buffer_index		Index of the command buffer to reset
	*	@param	release				Release command buffer resources in addition to reset
	*/
	void reset(std::uint32_t buffer_index, bool release = false) {
		auto &fence = fences[buffer_index];
		auto &buffer = buffers[buffer_index];

		if (!fence.is_signaled())
			fence.wait_idle();
		fence.reset();

		if (!release)
			buffer.reset();
		else
			buffer.reset_release();
	}

	/**
	*	@brief	Enqueues a submit task that submits the command buffer to the queue
	*
	*	@param	buffer_index		Index of the command buffer to submit
	*	@param	wait_semaphores		See vk_queue::submit
	*	@param	signal_semaphores	See vk_queue::submit
	*/
	auto submit(std::uint32_t buffer_index,
				const std::vector<std::pair<vk_semaphore*, VkPipelineStageFlags>> &wait_semaphores,
				const std::vector<vk_semaphore*> &signal_semaphores) {
		return enqueue_with_buffer_idx<void>([=](std::uint32_t idx) -> void {
			auto &fence = fences[idx];
			auto &buffer = buffers[idx];

			queue.submit({ &buffer },
						 wait_semaphores,
						 signal_semaphores,
						 &fence);
		}, buffer_index);
	}

	auto &device_queue() const { return queue; }
	auto &queue_descriptor() const { return descriptor; }
	auto &get_command_buffers() const { return buffers; }
};

}
}
