// StE
// © Shlomi Steinberg, 2015-2017

#pragma once

#include <stdafx.hpp>
#include <ste_context.hpp>
#include <device_pipeline_shader_stage.hpp>

#include <vk_pipeline_graphics.hpp>
#include <vk_unique_descriptor_set.hpp>
#include <vk_descriptor_set_layout_binding.hpp>

#include <font.hpp>
#include <glyph_manager.hpp>
#include <glyph_point.hpp>

#include <attributed_string.hpp>

#include <memory>
#include <string>
#include <vector>
#include <vk_framebuffer.hpp>

namespace StE {
namespace Text {

class text_manager {
private:
	friend class text_renderer;

private:
	const ste_context &context;

	glyph_manager gm;
	font default_font;
	int default_size;

	ste_resource<GL::device_pipeline_shader_stage> vert;
	ste_resource<GL::device_pipeline_shader_stage> geom;
	ste_resource<GL::device_pipeline_shader_stage> frag;

	std::unique_ptr<GL::vk_unique_descriptor_set> descriptor_set;

private:
	static void adjust_line(std::vector<glyph_point> &, const attributed_wstring &, unsigned, float, float, const glm::vec2 &);
	std::vector<glyph_point> create_points(glm::vec2, const attributed_wstring &);

private:
	static GL::vk_unique_descriptor_set create_descriptor_set(const GL::vk_logical_device &, std::uint32_t);
	bool update_glyphs(GL::vk_command_recorder &recorder);

public:
	text_manager(const ste_context &context,
				 const font &default_font,
				 int default_size = 28);

	text_manager(text_manager&&) = default;

	std::unique_ptr<text_renderer> create_renderer(const GL::vk_render_pass *renderpass);
};

}
}
