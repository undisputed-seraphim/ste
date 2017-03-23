//	StE
// � Shlomi Steinberg 2015-2016

#pragma once

#include <stdafx.hpp>
#include <vulkan/vulkan.h>

#include <vector>

namespace StE {
namespace GL {

class vk_descriptor_set_copy_resources {
	friend class vk_descriptor_set;

private:
	VkDescriptorSet src;
	std::uint32_t src_binding_index;
	std::uint32_t src_array_element;
	std::uint32_t dst_binding_index;
	std::uint32_t dst_array_element;
	std::uint32_t count;

public:
	vk_descriptor_set_copy_resources(const VkDescriptorSet &src,
									 std::uint32_t src_binding_index,
									 std::uint32_t src_array_element,
									 std::uint32_t dst_binding_index,
									 std::uint32_t dst_array_element,
									 std::uint32_t count)
		: src(src),
		src_binding_index(src_binding_index),
		src_array_element(src_array_element),
		dst_binding_index(dst_binding_index),
		dst_array_element(dst_array_element),
		count(count)
	{}

	vk_descriptor_set_copy_resources(vk_descriptor_set_copy_resources &&) = default;
	vk_descriptor_set_copy_resources &operator=(vk_descriptor_set_copy_resources &&) = default;
	vk_descriptor_set_copy_resources(const vk_descriptor_set_copy_resources &) = default;
	vk_descriptor_set_copy_resources &operator=(const vk_descriptor_set_copy_resources &) = default;
};

}
}
