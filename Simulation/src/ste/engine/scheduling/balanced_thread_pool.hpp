// StE
// © Shlomi Steinberg, 2015-2016

#pragma once

#include "stdafx.hpp"

#include "concurrent_queue.hpp"
#include "interruptible_thread.hpp"
#include "function_wrapper.hpp"
#include "thread_pool_task.hpp"

#include "thread_constants.hpp"
#include "thread_priority.hpp"
#include "thread_affinity.hpp"

#include "system_times.hpp"

#include <atomic>
#include <mutex>
#include <future>
#include <condition_variable>

#include <vector>
#include <bitset>

#include <chrono>

namespace StE {

class balanced_thread_pool {
private:
	mutable std::mutex m;
	std::condition_variable notifier;
	std::vector<interruptible_thread> workers;
	std::vector<interruptible_thread> despawned_workers;

	concurrent_queue<unique_function_wrapper> task_queue;

	system_times sys_times;
	std::chrono::high_resolution_clock::time_point last_pool_balance;
	std::atomic<int> requests_pending{ 0 };
	std::atomic<int> active_workers{ 0 };
	int threads_sleeping{ 0 };

	float idle_time_threshold_for_new_worker;
	float kernel_time_thershold_for_despawn_extra_worker;
	float idle_time_threshold_for_despawn_surplus_worker;

private:
	void spawn_worker(int schedule_on_cpu = -1) {
		workers.emplace_back([this]() {
			std::unique_ptr<unique_function_wrapper> task;

			for (;;) {
				if (interruptible_thread::is_interruption_flag_set()) return;

				{
					std::unique_lock<std::mutex> l(this->m);

					++threads_sleeping;
					this->notifier.wait(l, [&]() {
						return interruptible_thread::is_interruption_flag_set() ||
							   (task = task_queue.pop()) != nullptr;
					});
					--threads_sleeping;
				}

				active_workers.fetch_add(1, std::memory_order_relaxed);

				while (task != nullptr) {
					run_task(std::move(*task));
					if (interruptible_thread::is_interruption_flag_set())
						return;
					task = task_queue.pop();
				}

				active_workers.fetch_add(-1, std::memory_order_relaxed);
			}
		});

		auto t = &workers.back().get_thread();

		thread_set_priority_low(t);
		if (schedule_on_cpu >= 0) {
			constexpr auto bits = sizeof(std::size_t) * 8;

			std::bitset<bits> mask(0);
			mask[schedule_on_cpu] = 1;
			thread_set_affinity<bits>(t, mask);
		}
	}

	void despawn_worker() {
		assert(workers.size());

		auto ref = std::move(workers.back());
		workers.pop_back();

		ref.interrupt();
		ref.detach();
		despawned_workers.push_back(std::move(ref));
	}

	void on_enqueue() {
		requests_pending.fetch_add(1, std::memory_order_release);
		notifier.notify_one();
	}

	void run_task(const unique_function_wrapper &&task) {
		requests_pending.fetch_add(-1, std::memory_order_relaxed);
		task();
	}

	unsigned min_worker_threads() const {
		assert(std::thread::hardware_concurrency());

		return std::max<unsigned>(std::thread::hardware_concurrency(), 1u);
	}

public:
	balanced_thread_pool(float idle_time_threshold_for_new_worker = .1f, float kernel_time_thershold_for_despawn_extra_worker = .1f, float idle_time_threshold_for_despawn_surplus_worker = .2f)
		: idle_time_threshold_for_new_worker(idle_time_threshold_for_new_worker),
		  kernel_time_thershold_for_despawn_extra_worker(kernel_time_thershold_for_despawn_extra_worker),
		  idle_time_threshold_for_despawn_surplus_worker(idle_time_threshold_for_despawn_surplus_worker) {
		int threads = min_worker_threads();
		int max_threads = std::thread::hardware_concurrency();
		for (int i = 0; i < threads; ++i)
			spawn_worker(max_threads - threads + i);
	}

	~balanced_thread_pool() {
		for (auto &t : workers)
			t.interrupt();
		do { notifier.notify_all(); } while (!m.try_lock());
		m.unlock();
		for (auto &t : workers)
			t.join();
		for (auto &t : despawned_workers)
			t.get_future().get();
	}

	balanced_thread_pool(balanced_thread_pool &&) = delete;
	balanced_thread_pool(const balanced_thread_pool &) = delete;
	balanced_thread_pool &operator=(balanced_thread_pool &&) = delete;
	balanced_thread_pool &operator=(const balanced_thread_pool &) = delete;

 	template <typename R>
 	std::future<R> enqueue(thread_pool_task<R> &&f) {
 		auto future = f.get_future();
 		task_queue.push(std::move(f));

		on_enqueue();

 		return future;
	}

	void load_balance() {
		assert(is_main_thread());

		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> time_since_last_pool_balance = now - last_pool_balance;
		if (time_since_last_pool_balance.count() < .05f)
			return;
		last_pool_balance = now;

		float idle_frac = .0f;
		float kernel_frac = .0f;
		float user_frac = .0f;
		if (!sys_times.get_times_since_last_call(idle_frac, kernel_frac, user_frac))
			return;

		// Cleanup despawn queue
		for (auto it = despawned_workers.begin(); it != despawned_workers.end(); ) {
			if (it->get_future().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				it->get_future().get();			// Might throw
				it = despawned_workers.erase(it);
			}
			else
				++it;
		}

		// Check for workers with exceptions
		for (auto it = workers.begin(); it != workers.end(); ) {
			if (it->get_future().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				it->get_future().get();
				it = workers.erase(it);
			}
			else
				++it;
		}

		unsigned min_threads = min_worker_threads();
		int req = requests_pending.load(std::memory_order_acquire);
		if (threads_sleeping == 0 &&
			idle_frac > idle_time_threshold_for_new_worker) {
			spawn_worker();
		}
		else if (workers.size() > min_threads &&
					(kernel_frac > kernel_time_thershold_for_despawn_extra_worker ||
				  	 (req == 0 && idle_frac > idle_time_threshold_for_despawn_surplus_worker) ||
				  	 threads_sleeping > 1)) {
			despawn_worker();
		}
	}

	int get_workers_count() const { return workers.size(); }
	int get_pending_requests_count() const { return requests_pending.load(std::memory_order_relaxed); }
	int get_active_workers_count() const { return active_workers.load(std::memory_order_relaxed); }
	int get_sleeping_workers_count() const { return threads_sleeping; }
};

}
