// StE
// � Shlomi Steinberg, 2015

#pragma once

#include <atomic>
#include <memory>

#include <functional>
#include <type_traits>

#include <array>
#include <vector>
#include <bitset>

#include <stdlib.h>

#include <immintrin.h>

#include "shared_double_reference_guard.h"

namespace StE {

template <typename K, typename V, int cache_line = 64>
class concurrent_unordered_map {
private:
	using Hasher = std::hash<K>;

public:
	using mapped_type = V;
	using key_type = K;

private:
	struct concurrent_map_bucket_data {
		using value_type = shared_double_reference_guard<mapped_type, true>;
		using value_data_guard_type = value_type::data_guard;

		key_type k;
		value_type *v;

		template <typename S, typename ... Ts>
		concurrent_map_bucket_data(S&& k, Ts&&... args) : k(std::forward<S>(k)), v(new value_type(std::forward<Ts>(args)...)) {}
		~concurrent_map_bucket_data() { delete v; }
	};

	static_assert(std::is_default_constructible<mapped_type>::value, "V must be default constructible.");

	static constexpr int bucket_size = sizeof(unsigned) + sizeof(concurrent_map_bucket_data*);
	static constexpr int N = cache_line / bucket_size - 1;
	static constexpr int depth_threshold = 1;
	static constexpr float min_load_factor_for_resize = .5f;

	static_assert((cache_line % bucket_size) == 0, "cache_line is indivisible by sizeof(bucket_type)");
	static_assert(N > 2, "cache_line can't hold enough buckets");

	struct concurrent_map_virtual_bucket {
		std::array<std::atomic<unsigned>, N> hash{ std::atomic<unsigned>(0) };
		std::array<std::atomic<concurrent_map_bucket_data*>, N> buckets{ std::atomic<concurrent_map_bucket_data*>(nullptr) };

		std::atomic<concurrent_map_virtual_bucket*> next{ nullptr };
		void *_unused;

		~concurrent_map_virtual_bucket() {
			auto ptr = next.load();
			if (ptr) 
				delete ptr;
			for (auto &b : buckets) {
				auto ptr = b.load();
				if (ptr) delete ptr;
			}
		}
	};

	static_assert(cache_line == sizeof(concurrent_map_virtual_bucket), "Nonsensical padding on concurrent_map_virtual_bucket");

	using virtual_bucket_type = concurrent_map_virtual_bucket;
	using hash_table_type = virtual_bucket_type*;

	template <class table_ptr>
	struct resize_data_struct {
		static constexpr int chunk_size = 8;

		unsigned size;
		hash_table_type buckets;
		shared_double_reference_guard<table_ptr, false>::data_guard new_table_guard{ nullptr };
		std::vector<std::atomic<int>> markers;

		resize_data_struct(unsigned size, unsigned old_size) : size(size), buckets(table_ptr::alloc(size)), markers((old_size + chunk_size - 1) / chunk_size) {
			assert(markers[0].is_lock_free() && "markers not lock free");
		}
	};

	struct buckets_ptr {
		unsigned size;
		hash_table_type buckets{ nullptr };
		shared_double_reference_guard<resize_data_struct<buckets_ptr>, false> resize_ptr;

		static hash_table_type alloc(unsigned size) {
			auto b = reinterpret_cast<virtual_bucket_type*>(_aligned_malloc(sizeof(virtual_bucket_type) * size, cache_line));
			new (b) virtual_bucket_type[size];
			assert(b[0].buckets[0].is_lock_free() && "bucket_type not lock free");
			return b;
		}

		buckets_ptr(unsigned size, hash_table_type table) : size(size), buckets(table) {}
		buckets_ptr(unsigned size) : size(size), buckets(alloc(size)) {}
		~buckets_ptr() {
			resize_ptr.drop();
		}

		buckets_ptr(const buckets_ptr &b) = delete;
		buckets_ptr &operator=(const buckets_ptr &b) = delete;
	};

	using resize_data = resize_data_struct<buckets_ptr>;
	using resize_data_guard_type = shared_double_reference_guard<resize_data, false>::data_guard;
	using hash_table_guard_type = shared_double_reference_guard<buckets_ptr, false>::data_guard;

public:
	using value_data_guard_type = concurrent_map_bucket_data::value_data_guard_type;

private:
	mutable shared_double_reference_guard<buckets_ptr, false> hash_table;
	std::atomic<int> items{ 0 };

	template <typename T>
	unsigned __fastcall hash_function(T&& t) const { auto h = Hasher()(std::forward<T>(t)); return h ? h : 1; }

	void __fastcall copy_to_new_table(resize_data_guard_type &resize_guard, unsigned hash, const key_type &key, const mapped_type &val) {
		unsigned mask = resize_guard->size - 1;
		unsigned i = hash & mask;
		auto &virtual_bucket = resize_guard->buckets[i];

		insert_update_into_virtual_bucket(virtual_bucket, hash, key, .0f, false, 1, val);
	}

	void __fastcall copy_virtual_bucket(hash_table_guard_type &old_table_guard,
										resize_data_guard_type &resize_guard,
										int i, int chunks) {
		int chunk_size = resize_data::chunk_size;
		int j = i * chunk_size;
		for (int k = 0; k < chunk_size && j < old_table_guard->size; ++k, ++j) {
			auto *virtual_bucket = &old_table_guard->buckets[j];
			for (;;) {
				for (int b = 0; b < N; ++b) {
					auto bucket = virtual_bucket->buckets[b].load();
					if (bucket) {
						auto val_guard = bucket->v->acquire();
						val_guard.is_valid() ?
							copy_to_new_table(resize_guard, virtual_bucket->hash[b].load(), bucket->k, *val_guard) :
							items.fetch_sub(1, std::memory_order_relaxed);
					}
				}

				virtual_bucket = virtual_bucket->next.load();
				if (!virtual_bucket)
					break;
			}
		}
	}

	template <typename ... Ts>
	void __fastcall resize_with_pending_insert(hash_table_guard_type &old_table_guard,
											   resize_data_guard_type &resize_guard,
											   unsigned hash, 
											   const key_type &key, 
											   bool helper_only,
											   bool delete_item,
											   Ts&&... val_args) {
		if (!resize_guard.is_valid())
			resize_guard = old_table_guard->resize_ptr.acquire();
		if (!resize_guard.is_valid()) {
			if (helper_only)
				return;
			unsigned new_size = 2 * old_table_guard->size;
			while (!resize_guard.is_valid()) {
				old_table_guard->resize_ptr.try_compare_emplace(std::memory_order_acq_rel, resize_guard, new_size, old_table_guard->size);
				resize_guard = old_table_guard->resize_ptr.acquire();
				assert(resize_guard.is_valid());
			}
		}

		unsigned mask = resize_guard->size - 1;
		unsigned i = hash & mask;
		auto &virtual_bucket = resize_guard->buckets[i];
		!delete_item ?
			insert_update_into_virtual_bucket(virtual_bucket, hash, key, .0f, true, 1, std::forward<Ts>(val_args)...) :
			remove_from_virtual_bucket(virtual_bucket, hash, key);

		int chunks = resize_guard->markers.size();
		for (int i = 0; i < chunks; ++i) {
			int old_val = 0;
			if (!resize_guard->markers[i].compare_exchange_strong(old_val, 1, std::memory_order_acq_rel, std::memory_order_relaxed))
				continue;
			copy_virtual_bucket(old_table_guard, resize_guard, i, chunks);
			resize_guard->markers[i].store(2, std::memory_order_release);
		}

		bool old_table_moved = true;
		for (int i = 0; i < chunks; ++i)
			if (!(old_table_moved &= resize_guard->markers[i].load(std::memory_order_acquire) == 2)) break;
		if (old_table_moved) {
			if (hash_table.try_compare_emplace(std::memory_order_acq_rel, old_table_guard, resize_guard->size, resize_guard->buckets)) {
				auto ptr = old_table_guard->buckets;
				for (int i = 0; i < old_table_guard->size; ++i)
					(&ptr[i])->~virtual_bucket_type();
				_aligned_free(ptr);
			}
		}
	}

	int __fastcall find_hash_in_virtual_bucket(virtual_bucket_type *virtual_bucket, unsigned hash) const {
		static_assert(N == 7 && sizeof(unsigned) == 4 && cache_line == 64, "AVX2 optimization. Designed for a 32-bit compilation targeting any AVX2 capable architecture, like the 4th-generation Intel Core.");
		auto ptr = reinterpret_cast<__m256i*>(&virtual_bucket->hash[0]);
		__m256i m256_hash = _mm256_set1_epi32(*reinterpret_cast<int*>(&hash));
		__m256i m256_comparison = _mm256_cmpeq_epi32(m256_hash, *ptr);
		int bitmask = _mm256_movemask_ps(*reinterpret_cast<__m256*>(&m256_comparison));
		int bsf = _bit_scan_forward(bitmask);
		return !bitmask ? -1 : std::min(6, bsf);

		// Boring girly version
		// 		for (int j = 0; j < N; ++j) {
		// 			auto bucket = virtual_bucket->buckets[pos].load(std::memory_order_acquire);
		// 			if (bucket.hash == hash)
		// 				return j;
		// 		}
	}

	bool __fastcall remove_from_virtual_bucket(concurrent_map_virtual_bucket &virtual_bucket, unsigned hash, const key_type &key) {
		int pos = find_hash_in_virtual_bucket(&virtual_bucket, hash);
		if (pos >= 0) {
			auto bucket = virtual_bucket.buckets[pos].load();
			if (bucket && bucket->k == key) {
				bucket->v->drop();
				return true;
			}
		}

		auto next_ptr = virtual_bucket.next.load();
		if (!next_ptr)
			return false;
		return remove_from_virtual_bucket(*next_ptr, hash, key);
	}

	template <typename ... Ts>
	bool __fastcall insert_update_into_virtual_bucket(concurrent_map_virtual_bucket &virtual_bucket,
													  unsigned hash, 
													  const key_type &key, 
													  float load_factor,
													  bool is_new_item_insert,
													  int depth,
													  Ts&&... val_args) {
		bool request_resize = false;
		for (int j = 0; j < N;) {
			auto bucket_data = virtual_bucket.buckets[j].load();
			if (!bucket_data) {
				unsigned old_hash = virtual_bucket.hash[j].load(std::memory_order_relaxed);
				if (virtual_bucket.hash[j].compare_exchange_strong(old_hash, hash, std::memory_order_acq_rel, std::memory_order_relaxed)) {
					virtual_bucket.buckets[j].store(new concurrent_map_bucket_data(key, std::forward<Ts>(val_args)...), std::memory_order_release);
					if (is_new_item_insert)
						items.fetch_add(1, std::memory_order_relaxed);
					return !request_resize;
				}

				continue;
			}

			unsigned old_hash = virtual_bucket.hash[j].load();
			if (old_hash != hash || bucket_data->k != key) {
				++j; continue;
			}
			if (!is_new_item_insert)
				return false;
			bucket_data->v->emplace(std::memory_order_release, std::forward<Ts>(val_args)...);
			return !request_resize;
		}

		if (load_factor >= min_load_factor_for_resize && depth >= depth_threshold) 
			request_resize = true;

		auto next_ptr = virtual_bucket.next.load();
		while (!next_ptr) {
			auto new_next = new virtual_bucket_type;
			if (virtual_bucket.next.compare_exchange_strong(next_ptr, new_next, std::memory_order_acq_rel, std::memory_order_acquire))
				next_ptr = new_next;
			assert(next_ptr);
		}

		return !request_resize && insert_update_into_virtual_bucket(*next_ptr, hash, key, load_factor, is_new_item_insert, depth + 1, std::forward<Ts>(val_args)...);
	}

public:
	concurrent_unordered_map() : hash_table(1024) {}
	~concurrent_unordered_map() { 
		auto data_guard = hash_table.acquire();
		auto ptr = data_guard->buckets;
		if (ptr) {
			for (int i = 0; i < data_guard->size; ++i)
				(&ptr[i])->~virtual_bucket_type();
			_aligned_free(ptr);
		}
	}

	template <typename ... Ts>
	void __fastcall emplace(const key_type &key, Ts&&... val_args) {
		unsigned hash = hash_function(key);

		auto table_guard = hash_table.acquire();
		unsigned mask = table_guard->size - 1;
		unsigned i = hash & mask;
		auto &virtual_bucket = table_guard->buckets[i];

		resize_data_guard_type resize_guard{ nullptr };
		if (!insert_update_into_virtual_bucket(virtual_bucket,
											   hash, key,
											   static_cast<float>(items.load(std::memory_order_relaxed)) / static_cast<float>(table_guard->size * N),
											   true, 1,
											   std::forward<Ts>(val_args)...)) {
			resize_with_pending_insert(table_guard, resize_guard, hash, key, false, false, std::forward<Ts>(val_args)...);
			return;
		}

		resize_guard = table_guard->resize_ptr.acquire();
		if (resize_guard.is_valid())
			resize_with_pending_insert(table_guard, resize_guard, hash, key, true, false, std::forward<Ts>(val_args)...);
	}

	void __fastcall remove(const key_type &key) {
		unsigned hash = hash_function(key);

		auto table_guard = hash_table.acquire();
		unsigned mask = table_guard->size - 1;
		unsigned i = hash & mask;
		remove_from_virtual_bucket(table_guard->buckets[i], hash, key);

		auto resize_guard = table_guard->resize_ptr.acquire();
		if (resize_guard.is_valid())
			resize_with_pending_insert(table_guard, resize_guard, hash, key, true, true);
	}

	const value_data_guard_type __fastcall try_get(const key_type &key) const {
		unsigned hash = hash_function(key);

		auto table_guard = hash_table.acquire();
		unsigned mask = table_guard->size - 1;
		unsigned i = hash & mask;

		auto *virtual_bucket = &table_guard->buckets[i];
		for (;;) {
			int pos = find_hash_in_virtual_bucket(virtual_bucket, hash);
			if (pos >= 0) {
				auto bucket = virtual_bucket->buckets[pos].load(std::memory_order_relaxed);
				if (bucket && bucket->k == key) 
					return bucket->v->acquire(std::memory_order_relaxed);
			}

			virtual_bucket = virtual_bucket->next.load(std::memory_order_relaxed);
			if (!virtual_bucket)
				return concurrent_map_bucket_data::value_data_guard_type(nullptr);
		}
	}

	auto operator[](const key_type &k) const {
		return try_get(k);
	}
};

}
