// StE
// � Shlomi Steinberg, 2015

#pragma once

#include <atomic>
#include <iostream>

#include "concurrent_pointer_recycler.h"

namespace StE {

template <typename DataType, bool recycle_pointers>
class atomic_double_ref_guard {
private:
	class data {
	public:
		std::atomic<int> *internal_counter;
		DataType object;

		template <typename ... Ts>
		data(Ts&&... args) : object(std::forward<Ts>(args)...), internal_counter(new std::atomic<int>(0)) {}
		data& operator=(data &&d) {
			internal_counter->store(0, std::memory_order_release);
			object = std::move(d.object);
			return *this;
		}
		~data() { delete internal_counter; }

		void release_ref() {
			if (internal_counter->fetch_add(1, std::memory_order_relaxed) == -1) {
				internal_counter->load(std::memory_order_acquire);
				destroy();
			}
		}

	private:
		template <bool b, typename Sfinae = void>
		class data_factory {
		public:
			void __fastcall release(data *ptr) { delete ptr; }
			template <typename ... Ts>
			data* __fastcall claim(Ts&&... args) { return new data(std::forward<Ts>(args)...); }
		};
		template <bool b>
		class data_factory<b, std::enable_if_t<b>> : public concurrent_pointer_recycler<data> {};

	public:
		static data_factory<recycle_pointers> recycler;

		void destroy() {
			recycler.release(this);
		}
	};

	struct data_ptr {
		int external_counter;
		data *ptr;
	};

public:
	class data_guard {
		friend class atomic_double_ref_guard<DataType, recycle_pointers>;

	private:
		data *ptr;

	public:
		data_guard(data *ptr) : ptr(ptr) { }
		data_guard(const data_guard &d) = delete;
		data_guard &operator=(const data_guard &d) = delete;
		data_guard(data_guard &&d) {
			ptr = d.ptr;
			d.ptr = nullptr; 
		}
		data_guard &operator=(data_guard &&d) {
			if (ptr) ptr->release_ref();
			ptr = d.ptr;
			d.ptr = nullptr; 
			return *this;
		}

		~data_guard() { if (ptr) ptr->release_ref(); }

		bool is_valid() { return !!ptr; }

		DataType* operator->() { return &ptr->object; }
		DataType& operator*() { return ptr->object; }
	};

private:
	std::atomic<data_ptr> guard;

	void __fastcall release(data_ptr &old_data_ptr) {
		if (!old_data_ptr.ptr)
			return;
		auto external = old_data_ptr.external_counter;
		if (old_data_ptr.ptr->internal_counter->fetch_sub(external, std::memory_order_acquire) == external - 1)
			old_data_ptr.ptr->destroy();
		else
			old_data_ptr.ptr->release_ref();
	}

public:
	atomic_double_ref_guard() {
		data_ptr new_data_ptr{ 0, nullptr };
		guard.store(new_data_ptr);
	}

	template <typename ... Ts>
	atomic_double_ref_guard(Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		guard.store(new_data_ptr);

		assert(guard.is_lock_free() && "guard not lock free");
	}

	~atomic_double_ref_guard() {
		data_ptr old_data_ptr = guard.load();
		release(old_data_ptr);
	}

	data_guard __fastcall acquire(std::memory_order order = std::memory_order_acquire) {
		data_ptr new_data_ptr;
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		do {
			new_data_ptr = old_data_ptr;
			++new_data_ptr.external_counter;
		} while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		return data_guard(new_data_ptr.ptr);
	}

	template <typename ... Ts>
	data_guard __fastcall emplace_and_acquire(std::memory_order order, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 2, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		release(old_data_ptr);

		return data_guard(new_data_ptr.ptr);
	}

	template <typename ... Ts>
	void __fastcall emplace(std::memory_order order, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed));

		release(old_data_ptr);
	}

	template <typename ... Ts>
	bool __fastcall try_emplace(std::memory_order order1, std::memory_order order2, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(order2);
		if (guard.compare_exchange_strong(old_data_ptr, new_data_ptr, order1, std::memory_order_relaxed)) {
			release(old_data_ptr);
			return true;
		}
		delete new_data;
		return false;
	}

	template <typename ... Ts>
	bool __fastcall try_compare_emplace(std::memory_order order, data_guard &old_data, Ts&&... args) {
		data *new_data = data::recycler.claim(std::forward<Ts>(args)...);
		data_ptr new_data_ptr{ 1, new_data };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);

		bool success = false;
		while (old_data_ptr.ptr == old_data.ptr && !(success = guard.compare_exchange_weak(old_data_ptr, new_data_ptr, order, std::memory_order_relaxed)));
		if (success)
			release(old_data_ptr);
		else
			delete new_data;

		return success;
	}

	bool __fastcall is_valid_hint(std::memory_order order = std::memory_order_relaxed) const {
		return !!guard.load(order).ptr;
	}

	void __fastcall drop() {
		data_ptr new_data_ptr{ 0, nullptr };
		data_ptr old_data_ptr = guard.load(std::memory_order_relaxed);
		while (!guard.compare_exchange_weak(old_data_ptr, new_data_ptr, std::memory_order_acq_rel, std::memory_order_relaxed));

		release(old_data_ptr);
	}
};

template <typename DataType, bool recycle_pointers>
atomic_double_ref_guard<DataType, recycle_pointers>::data::data_factory<recycle_pointers> atomic_double_ref_guard<DataType, recycle_pointers>::data::recycler;

}
