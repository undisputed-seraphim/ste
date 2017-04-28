//	StE
// � Shlomi Steinberg 2015-2017

#pragma once

#include <stdafx.hpp>
#include <device_image_exceptions.hpp>
#include <device_image_base.hpp>

#include <format.hpp>
#include <image_layout.hpp>
#include <image_usage.hpp>

#include <vk_image.hpp>
#include <device_resource.hpp>
#include <device_resource_allocation_policy.hpp>

#include <format_type_traits.hpp>
#include <image_type_traits.hpp>

#include <pipeline_barrier.hpp>
#include <cmd_pipeline_barrier.hpp>
#include <cmd_copy_image.hpp>
#include <cmd_blit_image.hpp>

#include <format_rtti.hpp>

namespace ste {
namespace gl {

template <int dimensions, class allocation_policy = device_resource_allocation_policy_device>
class device_image : public device_image_base,
	public device_resource<vk::vk_image, allocation_policy>
{
	using Base = device_resource<vk::vk_image, allocation_policy>;
	using size_type = typename image_extent_type<dimensions>::type;

public:
	static glm::uvec3 size_to_extent(const size_type &size) {
		glm::uvec3 extent = { 1,1,1 };
		for (int i = 0; i < dimensions; ++i)
			extent[i] = size[i];
		return extent;
	}

public:
	device_image(const ste_context &ctx,
				 const image_initial_layout &layout,
				 const format &image_format,
				 const size_type &size,
				 const image_usage &usage,
				 std::uint32_t mips = 1,
				 std::uint32_t layers = 1,
				 bool supports_cube_views = false,
				 bool optimal_tiling = true,
				 bool sparse = false)
		: Base(ctx,
			   layout,
			   static_cast<VkFormat>(image_format),
			   dimensions,
			   size_to_extent(size),
			   static_cast<VkImageUsageFlags>(usage),
			   mips,
			   layers,
			   supports_cube_views,
			   optimal_tiling,
			   sparse)
	{}

	device_image(device_image&&) = default;
	device_image &operator=(device_image&&) = default;

	format get_format() const override final {
		return static_cast<format>(get_image_handle().get_format());
	}
	const vk::vk_image& get_image_handle() const override final { return *this; }
};

/**
 *	@brief	Partial specialization for 2-dimensional images.
 */
template <class allocation_policy>
class device_image<2, allocation_policy> : public device_image_base,
	public device_resource<vk::vk_image, allocation_policy>
{
	using Base = device_resource<vk::vk_image, allocation_policy>;
	using size_type = glm::uvec2;

private:
	template <typename Image>
	static void copy_surface_to_image(const ste_context &ctx,
									  const Image &image,
									  const gli::texture2d &surface,
									  const format &image_format,
									  const ste_queue_selector<> &selector,
									  int image_texel_bytes) {
		auto layers = surface.layers();

		// Create staging image
		device_image<2, device_resource_allocation_policy_host_visible_coherent>
			staging_image(ctx, image_initial_layout::preinitialized,
						  image_format, surface.extent(), image_usage::transfer_src,
						  1, layers, false, false);
		auto staging_image_bytes = staging_image.get_underlying_memory().get_size();
		auto mmap_u8_ptr = staging_image.get_underlying_memory().mmap<glm::u8>(0, staging_image_bytes);

		auto subresource_layout = staging_image->get_image_subresource_layout(0);

		// Upload
		for (std::uint32_t m = 0; m < surface.levels(); ++m) {
			auto level = surface[m];

			auto size = level.extent();
			auto *ptr = static_cast<glm::u8*>(*mmap_u8_ptr) + subresource_layout.offset;
			const auto *src = reinterpret_cast<const glm::u8*>(level.data());

			VkImageAspectFlags aspect = static_cast<VkImageAspectFlags>(vk_format_aspect(image.get_format()));
			VkImageCopy range = {
				{ aspect, 0, 0, layers },
				{ 0, 0, 0 },
				{ aspect, m, 0, layers },
				{ 0, 0, 0 },
				{ static_cast<std::uint32_t>(size.x), static_cast<std::uint32_t>(size.y), 1 }
			};

			// Write mipmap level to staging
			if (subresource_layout.rowPitch == image_texel_bytes * size.x) {
				memcpy(ptr, src, static_cast<std::size_t>(size.y * subresource_layout.rowPitch));
			}
			else {
				for (auto y = 0; y < size.y; ++y) {
					memcpy(ptr, src, static_cast<std::size_t>(image_texel_bytes * size.x));
					ptr += subresource_layout.rowPitch;
					src += size.x * image_texel_bytes;
				}
			}

			// Create a batch
			auto batch = ctx.device().select_queue(selector)->allocate_batch();
			auto& command_buffer = batch->acquire_command_buffer();
			auto& fence = batch->get_fence();

			// Enqueue mipmap copy on a transfer queue
			ctx.device().enqueue(selector, [&]() {
				// Record and submit a one-time batch
				{
					auto recorder = command_buffer.record();

					// Move to transfer layouts
					auto barrier = pipeline_barrier(pipeline_stage::top_of_pipe,
													pipeline_stage::top_of_pipe,
													std::vector<image_memory_barrier>{
						image_memory_barrier(staging_image,
											 m == 0 ? image_layout::preinitialized : image_layout::transfer_src_optimal,
											 image_layout::transfer_src_optimal,
											 access_flags::host_write,
											 access_flags::transfer_read),
							image_memory_barrier(image,
												 m == 0 ? image_layout::undefined : image_layout::transfer_dst_optimal,
												 image_layout::transfer_dst_optimal,
												 access_flags::none,
												 access_flags::transfer_write) });
					recorder << cmd_pipeline_barrier(barrier);

					// Copy to image
					recorder << cmd_copy_image(staging_image.get(), image_layout::transfer_src_optimal,
											   image.get(), image_layout::transfer_dst_optimal,
											   { range });
				}

				ste_device_queue::submit_batch(std::move(batch));
			});

			// Wait for completion
			fence.get();
		}
	}

	template <typename Image>
	static void generate_image_mipmaps(const ste_context &ctx,
									   const Image &image,
									   const ste_queue_selector<> &selector,
									   const glm::ivec2 &size,
									   std::uint32_t mip_levels,
									   std::uint32_t start_level) {
		// Enqueue mipmap copy on a transfer queue
		auto enqueue_future = ctx.device().enqueue(selector, [&]() {
			auto m = start_level;

			auto batch = ste_device_queue::thread_allocate_batch();
			auto& command_buffer = batch->acquire_command_buffer();

			VkImageAspectFlags aspect = static_cast<VkImageAspectFlags>(vk_format_aspect(image.get_format()));

			// Record and submit a one-time batch
			{
				auto recorder = command_buffer.record();

				for (; m < mip_levels; ++m) {
					// Move to transfer layouts
					auto barrier = pipeline_barrier(pipeline_stage::top_of_pipe,
													pipeline_stage::top_of_pipe,
													{ image_memory_barrier(image,
																		   image_layout::undefined,
																		   image_layout::transfer_dst_optimal,
																		   access_flags::none,
																		   access_flags::transfer_write,
																		   m, 1, 0, 1),
													image_memory_barrier(image,
																		 image_layout::transfer_dst_optimal,
																		 image_layout::transfer_src_optimal,
																		 access_flags::transfer_write,
																		 access_flags::transfer_read,
																		 m - 1, 1, 0, 1) });
					recorder << cmd_pipeline_barrier(barrier);

					VkImageBlit range = {};
					range.srcSubresource = { aspect, m - 1, 0, 1 };
					range.dstSubresource = { aspect, m, 0, 1 };
					range.srcOffsets[1] = {
						std::max<std::int32_t>(1, size.x >> (m - 1)),
						std::max<std::int32_t>(1, size.y >> (m - 1)),
						1
					};
					range.dstOffsets[1] = {
						std::max<std::int32_t>(1, size.x >> m),
						std::max<std::int32_t>(1, size.y >> m),
						1
					};

					// Copy to image
					recorder << cmd_blit_image(image, image_layout::transfer_src_optimal,
											   image, image_layout::transfer_dst_optimal,
											   VK_FILTER_LINEAR, { range });
				}

				// Move last mipmap to src optimal layout
				auto barrier = pipeline_barrier(pipeline_stage::top_of_pipe,
												pipeline_stage::top_of_pipe,
												image_memory_barrier(image,
																	 image_layout::transfer_dst_optimal,
																	 image_layout::transfer_src_optimal,
																	 access_flags::transfer_write,
																	 access_flags::transfer_read,
																	 mip_levels - 1, 1, 0, 1));
				recorder << cmd_pipeline_barrier(barrier);
			}

			ste_device_queue::submit_batch(std::move(batch));
		});

		enqueue_future.get();
	}

public:
	template <format image_format>
	static auto create_image_2d(const ste_context &ctx,
								gli::texture2d &&surface,
								const image_usage &usage,
								bool generate_mipmaps) {
		using image_element_type = typename format_traits<image_format>::element_type;
		static constexpr int image_texel_bytes = format_traits<image_format>::texel_bytes;
		static constexpr gli::format image_gli_format = format_traits<image_format>::gli_format;

		auto src_format = surface.format();
		std::uint32_t mip_levels = generate_mipmaps ? gli::levels(surface.extent()) : surface.levels();
		auto layers = surface.layers();
		auto size = surface.extent();
		if (src_format != image_gli_format) {
			throw device_image_format_exception("Input image_format is different from specified image format");
		}

		auto selector = make_primary_queue_selector();

		// Staging area
		device_image<2, device_resource_allocation_policy_device>
			image(ctx, image_initial_layout::unused,
				  image_format, size, image_usage::transfer_dst | image_usage::transfer_src | usage,
				  mip_levels, layers);

		// Copy from surface
		copy_surface_to_image(ctx, image, surface, image_format, selector, image_texel_bytes);

		// Generate remaining mipmaps, as needed
		std::uint32_t m = surface.levels();
		if (m < mip_levels) {
			generate_image_mipmaps(ctx,
								   image,
								   selector,
								   size,
								   mip_levels,
								   m);
		}

		return image;
	}

public:
	device_image(const ste_context &ctx,
				 const image_initial_layout &layout,
				 const format &image_format,
				 const size_type &size,
				 const image_usage &usage,
				 std::uint32_t mips = 1,
				 std::uint32_t layers = 1,
				 bool supports_cube_views = false,
				 bool optimal_tiling = true,
				 bool sparse = false)
		: Base(ctx,
			   layout,
			   static_cast<VkFormat>(image_format),
			   2,
			   glm::uvec3(size.x, size.y, 1),
			   static_cast<VkImageUsageFlags>(usage),
			   mips,
			   layers,
			   supports_cube_views,
			   optimal_tiling,
			   sparse)
	{}
	~device_image() noexcept {}

	device_image(device_image&&) = default;
	device_image &operator=(device_image&&) = default;

	format get_format() const override final {
		return static_cast<format>(get_image_handle().get_format());
	}
	const vk::vk_image& get_image_handle() const override final { return *this; };
};

}
}
