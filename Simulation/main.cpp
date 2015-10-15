
#include "stdafx.h"
#include "windows.h"

#include <glm/glm.hpp>

#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <thread>
#include <numeric>

#include "gl_utils.h"
#include "Log.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "StEngineControl.h"
#include "ModelLoader.h"
#include "Camera.h"
#include "GLSLProgram.h"
#include "GLSLProgramLoader.h"
#include "SurfaceIO.h"
#include "VertexBufferObject.h"
#include "VertexArrayObject.h"
#include "FramebufferObject.h"
#include "RenderTarget.h"
#include "Texture2DArray.h"
#include "PixelBufferObject.h"
#include "AtomicCounterBufferObject.h"
#include "Scene.h"
#include "TextRenderer.h"
#include "AttributedString.h"

using namespace StE::LLR;
using namespace StE::Text;

struct Vertex {
	glm::vec3 p;
	glm::vec2 t;
};

StE::LLR::Camera camera;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdParam, int iCmdShow) {
	StE::Log logger("Simulation");
	logger.redirect_std_outputs();
	ste_log_set_global_logger(&logger);
	ste_log() << "Simulation is running";

	float w = 1400, h = 900;
	constexpr float clip_far = 1000.f;
	constexpr float clip_near = 1.f;

	gl_context::context_settings settings;
	settings.vsync = false;
	StE::StEngineControl rc(std::make_unique<gl_context>(settings, "Shlomi Steinberg - Simulation", glm::i32vec2{ w, h }));

	std::string gl_err_desc;
	//while (StE::LLR::opengl::query_gl_error(gl_err_desc));
// 	StE::Graphics::texture_pool tp;
// 	StE::LLR::Texture2D tt(gli::format::FORMAT_RGBA8_UNORM, StE::LLR::Texture2D::size_type(256, 256), 5);
// 	StE::LLR::opengl::query_gl_error(gl_err_desc);
// 	auto itt = tp.commit(tt);
// 	tp.commit(tt);
// 	tp.uncommit(itt);
 	//while (StE::LLR::opengl::query_gl_error(gl_err_desc));

	rc.set_clipping_planes(clip_near, clip_far);
	camera.set_position({ -91.0412979, 105.631607, -60.2330551 });
	camera.lookat({ -91.9486542, 105.291336, -59.98624 });

	// Prepare
	StE::Graphics::Scene scene;
	StE::Text::TextRenderer text_renderer(rc, StE::Text::Font("Data/calligraph421-bt-roman.ttf"));

	std::unique_ptr<StE::LLR::GLSLProgram> transform = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "transform.vert", "frag.frag" })();
	std::unique_ptr<StE::LLR::GLSLProgram> deffered = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "passthrough_light.vert", "lighting.frag" })();
	std::unique_ptr<StE::LLR::GLSLProgram> hdr_create_histogram = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "hdr_create_histogram.glsl" })();
	std::unique_ptr<StE::LLR::GLSLProgram> hdr_compute_histogram_sums = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "hdr_compute_histogram_sums.glsl" })();
	std::unique_ptr<StE::LLR::GLSLProgram> hdr_compute_minmax = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "passthrough.vert","hdr_compute_minmax.frag" })();
	std::unique_ptr<StE::LLR::GLSLProgram> hdr_tonemap = StE::Resource::GLSLProgramLoader::load_program_task(rc, { "passthrough.vert","hdr_tonemap.frag" })();

	StE::LLR::RenderTarget depth_output(gli::format::FORMAT_D24_UNORM, StE::LLR::Texture2D::size_type(w, h));
	StE::LLR::Texture2D normal_output(gli::format::FORMAT_RGB32_SFLOAT, StE::LLR::Texture2D::size_type(w, h), 1);
	StE::LLR::Texture2D position_output(gli::format::FORMAT_RGB32_SFLOAT, StE::LLR::Texture2D::size_type(w, h), 1);
	StE::LLR::Texture2D color_output(gli::format::FORMAT_RGB32_SFLOAT, StE::LLR::Texture2D::size_type(w, h), 1);
	StE::LLR::FramebufferObject fbo;
	fbo.depth_binding_point() = depth_output;
	fbo[0] = position_output[0];
	fbo[1] = color_output[0];
	fbo[2] = normal_output[0];
	1_color_idx = fbo[0];
	0_color_idx = fbo[1];
	2_color_idx = fbo[2];

	using vertex_descriptor = StE::LLR::VBODescriptorWithTypes<glm::vec3, glm::vec2>::descriptor;
	using vbo_type = StE::LLR::VertexBufferObject<Vertex, vertex_descriptor>;
	std::shared_ptr<vbo_type> vbo = std::make_shared<vbo_type>(std::vector<Vertex>(
		{ { { -1.f, -1.f, .0f }, { .0f, .0f } },
		{ { 1.f, -1.f, .0f }, { 1.f, .0f } },
		{ { -1.f, 1.f, .0f }, { .0f, 1.f } },
		{ { 1.f, 1.f, .0f }, { 1.f, 1.f } } }
	));
	StE::LLR::VertexArrayObject vao;
	vao[0] = (*vbo)[1];
	vao[1] = (*vbo)[0];

 	StE::LLR::Texture2D hdr_image(gli::format::FORMAT_RGB32_SFLOAT, StE::LLR::Texture2D::size_type(w, h), 1);
 	StE::LLR::FramebufferObject fbo_hdr_image;
 	fbo_hdr_image[0] = hdr_image[0];

	Sampler linear_sampler;
	linear_sampler.set_min_filter(TextureFiltering::Linear);
	linear_sampler.set_mag_filter(TextureFiltering::Linear);

	int luminance_w = 1, luminance_h = 1;
	while ((luminance_w << 1) < w)
		luminance_w <<= 1;
	while ((luminance_h << 1) < h)
		luminance_h <<= 1;

 	StE::LLR::Texture2D histogram_minmax(gli::format::FORMAT_R32_SINT, StE::LLR::Texture2D::size_type(2, 1), 1);
 	StE::LLR::Texture2D hdr_lums(gli::format::FORMAT_R32_SFLOAT, StE::LLR::Texture2D::size_type(luminance_w, luminance_h), 1);
 	StE::LLR::FramebufferObject fbo_hdr_lums;
 	fbo_hdr_lums[0] = hdr_lums[0];
 	float big_float = 10000.f;
 	StE::LLR::PixelBufferObject<unsigned> histogram_minmax_eraser(std::vector<unsigned>{ *reinterpret_cast<unsigned*>(&big_float), 0 });
 	StE::LLR::AtomicCounterBufferObject<> histogram(64);
 	StE::LLR::ShaderStorageBuffer<unsigned> histogram_sums(64);

	bool running = true;

	// Bind input
	auto keyboard_listner = std::make_shared<decltype(rc)::hid_keyboard_signal_type::connection_type>(
		[&](StE::HID::keyboard::K key, int scanline, StE::HID::Status status, StE::HID::ModifierBits mods) {
		using namespace StE::HID;
		auto time_delta = rc.time_per_frame().count();

		if (status != Status::KeyDown)
			return;

		if (key == keyboard::K::KeyESCAPE)
			running = false;
		if (key == keyboard::K::KeyPRINT_SCREEN) {
			auto size = rc.gl()->framebuffer_size();
			gli::texture2D tex(gli::FORMAT_RGB8_UNORM, size);

			StE::LLR::FramebufferObject fbo;
			StE::LLR::Texture2D fbo_tex(gli::format::FORMAT_RGB8_UNORM, size, 1);
			fbo[0] = fbo_tex[0];

			rc.gl()->defaut_framebuffer().blit_to(fbo,size,size);
			fbo[0].read_pixels(tex.data(), 3 * size.x * size.y);

			rc.scheduler().schedule_now(StE::Resource::SurfaceIO::write_surface_2d_task(tex, R"(D:\a.png)"));
		};
	});
	rc.hid_signal_keyboard().connect(keyboard_listner);

	rc.set_pointer_hidden(true);

	bool loaded = false;
	auto model_future = rc.scheduler().schedule_now(StE::Resource::ModelLoader::load_model_task(R"(data\models\sponza.obj)", &scene));

	// Run main loop
	while (!loaded && running) {
		rc.run_loop();
		rc.gl()->clear_framebuffer();

		{
			using namespace StE::Text::Attributes;
			AttributedWString str = center(purple(huge(b(L"Loading Simulation..."))) + 
											L"\n" + 
											orange(large(L"By Shlomi Steinberg")));
 			text_renderer.render({ w / 2, h / 2 - 20 }, str);
 			text_renderer.render({ 10, 20 }, b(L"Thread pool workers: ") + 
 									olive(std::to_wstring(rc.scheduler().get_sleeping_workers())) + 
 									L"/" + 
 									olive(std::to_wstring(rc.scheduler().get_workers_count())));
		}

		if (model_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			continue;

		loaded = true;
	}

	while (running) {
		if (!rc.run_loop()) break;

		if (rc.window_active()) {
			auto time_delta = rc.time_per_frame().count();

			using namespace StE::HID;
			constexpr float movement_factor = 35.f;
			if (rc.get_key_status(keyboard::K::KeyW) == Status::KeyDown)
				camera.step_forward(time_delta*movement_factor);
			if (rc.get_key_status(keyboard::K::KeyS) == Status::KeyDown)
				camera.step_backward(time_delta*movement_factor);
			if (rc.get_key_status(keyboard::K::KeyA) == Status::KeyDown)
				camera.step_left(time_delta*movement_factor);
			if (rc.get_key_status(keyboard::K::KeyD) == Status::KeyDown)
				camera.step_right(time_delta*movement_factor);

			constexpr float rotation_factor = .05f;
			auto pp = rc.get_pointer_position();
			auto center = static_cast<glm::vec2>(rc.get_backbuffer_size())*.5f;
			rc.set_pointer_position(static_cast<glm::ivec2>(center));
			auto diff_v = (center - static_cast<decltype(center)>(pp)) * time_delta * rotation_factor;
			camera.pitch_and_yaw(-diff_v.y, diff_v.x); 
		}

		auto proj_mat = rc.projection_matrix();

		rc.gl()->enable_depth_test();
		fbo.bind();
		rc.gl()->clear_framebuffer(false);

		transform->bind();
		auto mv = camera.view_matrix() * glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-100, -35, 0)), glm::vec3(.25, .25, .25));
		transform->set_uniform("view_model", mv);
		transform->set_uniform("trans_inverse_view_model", glm::transpose(glm::inverse(mv)));
		transform->set_uniform("projection", proj_mat);
		scene.render();

		rc.gl()->disable_depth_test();

		vao.bind();

		fbo_hdr_image.bind();
		deffered->bind();
		deffered->set_uniform("view", camera.view_matrix());
		0_tex_unit = normal_output;
		1_tex_unit = position_output;
		3_tex_unit = color_output;
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		0_sampler_idx = linear_sampler;
		histogram_minmax_eraser >> histogram_minmax;
		unsigned zero = 0;
		histogram.clear(gli::FORMAT_R32_UINT, &zero);


		0_tex_unit = hdr_image;
		glViewport(0, 0, luminance_w, luminance_h);

		hdr_compute_minmax->bind();
		fbo_hdr_lums.bind();
		0_image_idx = histogram_minmax[0];
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		hdr_create_histogram->bind();
		1_tex_unit = hdr_lums;
		0_atomic_idx = histogram;
		glDispatchCompute(luminance_w / 32, luminance_h / 32, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		hdr_compute_histogram_sums->bind();
		hdr_compute_histogram_sums->set_uniform("hdr_lum_resolution", luminance_w * luminance_h);
		0_storage_idx = histogram_sums;
		1_storage_idx = buffer_object_cast<ShaderStorageBuffer<unsigned>>(histogram);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glViewport(0, 0, w, h);

		hdr_tonemap->bind();
		rc.gl()->defaut_framebuffer().bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		{
			using namespace StE::Text::Attributes;
 			text_renderer.render({ 30, h - 50 }, 
 								 blue(L"Frame time: ") + b(red(std::to_wstring(rc.time_per_frame().count()))) + L" ms");
		}
	}
	 
	return 0;
}
