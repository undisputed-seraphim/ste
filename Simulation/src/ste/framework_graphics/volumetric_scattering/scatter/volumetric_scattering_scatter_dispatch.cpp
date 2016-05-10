
#include "stdafx.hpp"
#include "volumetric_scattering_scatter_dispatch.hpp"

using namespace StE::Graphics;
using namespace StE::Core;

void volumetric_scattering_scatter_dispatch::set_context_state() const {
	GL::gl_current_context::get()->enable_state(StE::Core::GL::BasicStateName::TEXTURE_CUBE_MAP_SEAMLESS);

	ls->bind_lights_buffer(2);
	ls->bind_lights_transform_buffer(3);

	llls->bind_lll_buffer();

	8_tex_unit = *shadows_storage->get_cubemaps();
	8_sampler_idx = shadows_storage->get_shadow_sampler();

	7_image_idx = vss->get_volume_texture()->make_image().with_access(Core::ImageAccessMode::Write);

	program->bind();
}

void volumetric_scattering_scatter_dispatch::dispatch() const {
	const glm::ivec3 jobs = { 32, 32, 1 };
	auto size = (vss->get_size() + jobs - glm::ivec3(1)) / jobs;

	Core::GL::gl_current_context::get()->memory_barrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	Core::GL::gl_current_context::get()->dispatch_compute(size.x, size.y, size.z);
}
