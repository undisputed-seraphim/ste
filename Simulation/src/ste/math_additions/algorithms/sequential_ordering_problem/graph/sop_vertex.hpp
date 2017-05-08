// StE
// © Shlomi Steinberg, 2015-2016

#pragma once

#include <stdafx.hpp>
#include <graph_vertex.hpp>

#include <optional.hpp>

#include <boost_flatset.hpp>

namespace ste {
namespace algorithm {
namespace SOP {

template <typename V, typename E>
class sop_graph;
template <typename GraphType>
class sequential_ordering_problem;
template <typename G>
class sop_optimizer;

template <typename DepsContainer, typename RequisiteContainer = DepsContainer>
class sop_vertex : public graph::vertex {
	using Base = graph::vertex;
	
	template <typename V, typename E>
	friend class sop_graph;
	template <typename GraphType>
	friend class sequential_ordering_problem;
	template <typename G>
	friend class sop_optimizer;
	
private:
	// For optimization
	using DepsContainerT = DepsContainer;
	using MissingDepsContainerT = boost::container::flat_set<const sop_vertex<DepsContainer, RequisiteContainer>*>;
	
	mutable optional<MissingDepsContainerT> missing_deps;
	mutable bool visited{ false };
	
public:
	using Base::Base;
	virtual ~sop_vertex() noexcept {}
	
	virtual const DepsContainer &get_dependencies() const = 0;
	virtual const RequisiteContainer &get_requisite_for() const = 0;
};
	
}
}
}
