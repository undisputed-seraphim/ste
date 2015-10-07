// StE
// � Shlomi Steinberg, 2015

#pragma once

#include "lru_cache_cacheable.h"
#include "lru_cache_index.h"

#include "concurrent_queue.h"
#include "task.h"
#include "interruptible_thread.h"

#include <string>

#include <atomic>
#include <mutex>
#include <condition_variable>

#include <memory>

namespace StE {

template <typename K>
class lru_cache {
private:
	using key_type = K;
	using index_type = lru_cache_index<key_type>;
	using cacheable = index_type::val_type;

private:
	index_type index;

	mutable std::mutex m;
	mutable std::condition_variable cv;
	interruptible_thread t;

	mutable concurrent_queue<index_type::val_data_guard> accessed_queue;

	std::atomic<std::size_t> total_size{ 0 };
	std::size_t quota;
	boost::filesystem::path path;

private:
	void shutdown() {
		t.interrupt();
		do { cv.notify_one(); } while (!m.try_lock());
		m.unlock();
		t.join();
	}

	void item_accessed(index_type::val_data_guard &&val_guard) const {
		accessed_queue.push(std::move(val_guard));
		cv.notify_one();
	}

public:
	lru_cache(lru_cache &&) = delete;
	lru_cache(const lru_cache &) = delete;
	lru_cache &operator=(lru_cache &&) = delete;
	lru_cache &operator=(const lru_cache &) = delete;

	lru_cache(const boost::filesystem::path &path, std::size_t quota) : path(path), quota(quota), index(path), t([this]() {
		auto flag = interruptible_thread::interruption_flag;
		for (;;) {
			if (flag->is_set()) return;
			{
				std::unique_lock<std::mutex> l(this->m);
				this->cv.wait(l);
			}

			auto accessed_item = accessed_queue.pop();
			while (accessed_item != nullptr) {
				index.move_to_lru_front(**accessed_item);
				accessed_item = accessed_queue.pop();
			}

			std::size_t ts = this->total_size.load(std::memory_order_relaxed);
			while (ts > this->quota) {
				auto size = this->index.erase_back();
				assert(size);
				if (!size)
					break;

				ts = this->total_size.fetch_sub(size, std::memory_order_relaxed);
			}
		}
	}) {
		boost::filesystem::create_directory(path);
	}
	~lru_cache() { shutdown(); }

	template <typename V>
	void insert(const key_type &k, V &&v) {
		{
			cacheable item(k, path);
			item.archive(std::forward<V>(v));
			index.map.emplace(k, std::move(item));
		}

		std::atomic_thread_fence(std::memory_order_acquire);
		auto val_guard = index.map.try_get(k);
		if (!val_guard.is_valid())
			return;

		auto item_size = val_guard->get_size();
		assert(item_size);
		total_size.fetch_add(item_size, std::memory_order_relaxed);
		item_accessed(std::move(val_guard));
	}

	template <typename V>
	task<optional<V>> get(const key_type &k) const {
		return [=]() -> optional<V> {
			auto val_guard = this->index.map[k];
			if (!val_guard.is_valid())
				return none;

			auto v = val_guard->unarchive<V>();
			this->item_accessed(std::move(val_guard));
			return v;
		};
	}
};

}
