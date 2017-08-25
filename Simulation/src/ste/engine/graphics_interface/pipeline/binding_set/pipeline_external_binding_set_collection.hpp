//	StE
// � Shlomi Steinberg 2015-2017

#pragma once

#include <stdafx.hpp>
#include <pipeline_binding_set_pool.hpp>
#include <pipeline_resource_binding_queue.hpp>
#include <pipeline_layout_set_index.hpp>
#include <pipeline_layout_exceptions.hpp>

#include <pipeline_external_binding_layout.hpp>
#include <pipeline_external_binding_set.hpp>

#include <pipeline_binding_set_collection_cmd_bind.hpp>

#include <vk_shader.hpp>

#include <lib/flat_map.hpp>
#include <alias.hpp>
#include <signal.hpp>

namespace ste {
namespace gl {

class pipeline_external_binding_set_collection {
	friend class pipeline_external_resource_bind_point;
	friend class pipeline_layout;

private:
	using layout_t = pipeline_external_binding_set_layout;
	using set_layouts_t = lib::vector<layout_t>;
	
	using collection_t = lib::flat_map<pipeline_layout_set_index, pipeline_external_binding_set>;
	using name_binding_map_t = lib::flat_map<lib::string, const pipeline_external_binding_layout*>;

	using spec_to_dependant_array_variables_map_t = lib::flat_map<const ste_shader_stage_variable*, lib::vector<pipeline_layout_set_index>>;
	using spec_map_t = lib::unordered_map<ste_shader_program_stage, vk::vk_shader<>::spec_map>;

	using cmd_bind_t = pipeline_binding_set_collection_cmd_bind<pipeline_external_binding_set_collection>;

private:
	collection_t sets;
	set_layouts_t layouts;
	alias<pipeline_binding_set_pool> pool;

	name_binding_map_t name_map;

	name_binding_map_t spec_variables_map;
	spec_to_dependant_array_variables_map_t spec_to_dependant_array_variables_map;
	spec_map_t specializations;
	mutable signal<pipeline_external_binding_set_collection*> signal_specialization_change;
	mutable signal<pipeline_external_binding_set_collection*, const lib::vector<pipeline_layout_set_index>&> signal_set_invalidated;

	pipeline_resource_binding_queue binding_queue;

private:
	/**
	*	@brief	Updates the binding set with resource bindings
	*/
	void write_binding_queue() {
		if (binding_queue.empty())
			return;

		for (auto &b : binding_queue) {
			auto &idx = b.first;
			auto &writes = b.second;

			auto it = sets.find(idx);
			if (it == sets.end()) {
				// Set not found?!
				assert(false);
				continue;
			}

			auto &set = it->second;

			set.write(writes);
		}

		binding_queue.clear();
	}

	template <typename T>
	void specialize_constant_impl(const lib::string &name,
								  const optional<lib::string> &data = none,
								  const T *value = nullptr) {
		// Specialize the binding scalar variable
		// Used for dynamic array lengths
		auto it = spec_variables_map.find(name);
		if (it == spec_variables_map.end()) {
			throw pipeline_layout_variable_not_found_exception("Specialization constant with provided name not found");
		}

		auto &b = it->second->get_binding();
		auto *var = b.variable.get();
		auto *ptr = dynamic_cast<ste_shader_stage_variable_scalar*>(var);
		assert(ptr);
		if (ptr) {
			// Do nothing if value unchanged
			if (data) {
				if (ptr->read_specialized_value<T>() == *value)
					return;
				ptr->specialize_bin(data.get());
			}
			else {
				if (!ptr->has_non_default_specialization())
					return;
				ptr->reset_specialization();
			}
		}

		// Update shader stage specializations
		for (auto &s : it->second->stage_collection()) {
			auto& map = specializations[s];
			if (data)
				map[b.bind_idx] = data.get();
			else
				map.erase(b.bind_idx);
		}

		// Signal specialization constant change
		signal_specialization_change.emit(this);

		// If some array variable depends on this specialization constant, we need to update the relevant set descriptor layouts
		auto dependant_arrays_it = spec_to_dependant_array_variables_map.find(var);
		if (dependant_arrays_it != spec_to_dependant_array_variables_map.end()) {
			// Signal set invalidation, the set should be recreated.
			signal_set_invalidated.emit(this, dependant_arrays_it->second);
		}
	}

	/**
	*	@brief	Specializes constant at a binding point to a value
	*
	*	@throws	pipeline_layout_variable_not_found_exception	If specialization constant not found
	*/
	template <typename T>
	void specialize_constant(const lib::string &name, const T &value) {
		using S = std::remove_cv_t<std::remove_reference_t<T>>;
		static_assert(std::is_pod_v<S> || is_arithmetic_v<S>, "T must be a POD or arithmetic type");

		lib::string data;
		data.resize(sizeof(S));
		memcpy(data.data(), &value, sizeof(S));

		specialize_constant_impl<S>(name,
									std::move(data),
									&value);
	}
	/**
	*	@brief	Remove a specialization of constant at a binding point
	*	
	*	@throws	pipeline_layout_variable_not_found_exception	If specialization constant not found
	*/
	void remove_specialization(const lib::string &name) {
		specialize_constant_impl<unsigned>(name);
	}

public:
	pipeline_external_binding_set_collection(set_layouts_t &&layouts,
											 pipeline_binding_set_pool &pool)
		: layouts(std::move(layouts)),
		pool(pool)
	{
		lib::vector<pipeline_layout_set_index> indices;
		lib::vector<const layout_t*> layout_ptrs;
		indices.reserve(this->layouts.size());
		layout_ptrs.reserve(this->layouts.size());

		for (auto &l : this->layouts) {
			indices.push_back(l.get_set_index());
			layout_ptrs.push_back(&l);

			for(auto &b : l) {
				if (b.get_binding().binding_type == ste_shader_stage_binding_type::push_constant) {
					throw pipeline_layout_push_constant_in_external_set_exception("External binding sets can not containt push constants");
				}

				// Check for duplicate names
				auto ret = name_map.try_emplace(b.name(), &b);
				if (!ret.second) {
					// Name already exists
					throw pipeline_layout_duplicate_variable_name_exception("Variable's name already exists in collection");
				}

				// Individually map specialization constants
				if (b.get_binding().binding_type == ste_shader_stage_binding_type::spec_constant) {
					spec_variables_map[b.name()] = &b;
				}

				// Map array variables whose length depends on specialization constants
				auto var_arr = dynamic_cast<const ste_shader_stage_variable_array*>(b.get_binding().variable.get());
				if (var_arr && var_arr->length_spec_constant()) {
					spec_to_dependant_array_variables_map[var_arr->get_length_spec_constant_var()].push_back(b.set_idx());
				}
			}
		}

		// Allocate
		auto allocated_sets = pool.allocate_binding_sets<layout_t>(layout_ptrs);

		// Sort
		for (std::size_t i = 0; i < allocated_sets.size(); ++i) {
			auto idx = indices[i];
			sets.emplace(idx, std::move(allocated_sets[i]));
		}
	}
	~pipeline_external_binding_set_collection() noexcept {}


	pipeline_external_binding_set_collection(pipeline_external_binding_set_collection&&) = default;
	pipeline_external_binding_set_collection &operator=(pipeline_external_binding_set_collection&&) = default;

	/**
	*	@brief	Rebuilds the required sets
	*
	*	#return	Returns the old sets
	*/
	auto recreate_sets(const lib::vector<pipeline_layout_set_index> &set_indices) {
		lib::vector<pipeline_external_binding_set> ret_old_sets;

		lib::vector<const layout_t*> layout_ptrs;
		layout_ptrs.reserve(set_indices.size());
		for (auto &set_idx : set_indices) {
			// Define layout for new set
			auto l_it = sets.find(set_idx);
			if (l_it == sets.end()) {
				// Layout not found.
				assert(false);
				return ret_old_sets;
			}
			auto& s = l_it->second;
			layout_ptrs.push_back(&s.get_layout());
		}

		// Allocate the new sets
		lib::vector<pipeline_external_binding_set> new_sets = pool.get().allocate_binding_sets<layout_t>(layout_ptrs);
		ret_old_sets.reserve(new_sets.size());
		for (std::size_t i = 0; i<new_sets.size(); ++i) {
			auto &set_idx = set_indices[i];
			auto &new_set = new_sets[i];

			// Find old set
			auto it = sets.find(set_idx);
			if (it == sets.end()) {
				// Set not found.
				assert(false);
				continue;
			}
			auto &old_set = it->second;

			// Copy bindings from old set
			new_set.copy(old_set);

			// Save old and store new inplace, in sets collection
			ret_old_sets.push_back(std::move(old_set));
			it->second = std::move(new_set);
		}

		return ret_old_sets;
	}

	/**
	*	@brief	Creates a resource binder for a given variable name
	*/
	pipeline_external_resource_bind_point operator[](const lib::string &resource_name);

	auto& get_sets() const { return sets; }
	auto& get_layouts() const { return layouts; }

	/**
	 *	@brief	Updates resource writes
	 */
	void update() {
		write_binding_queue();
	}

	/**
	 *	@brief	Returns the specialization constant modified signal
	 */
	auto& get_signal_specialization_change() const { return signal_specialization_change; }

	/**
	 *	@brief	Returns the set invalidated signal
	 */
	auto& get_signal_set_invalidated() const { return signal_set_invalidated; }

	/**
	*	@brief	Binds the binding set collection
	*/
	auto cmd_bind(VkPipelineBindPoint bind_point,
				  const vk::vk_pipeline_layout<> *layout) const {
		return cmd_bind_t(this,
						  bind_point,
						  layout);
	}
};

}
}

#include <pipeline_external_resource_bind_point.hpp>
