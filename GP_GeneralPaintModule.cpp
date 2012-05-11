#include "GP_GeneralPaintModule.hpp"

#include "GP_VCycle.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#include <Vector/VectorGL.hpp>

#include <iostream>
#include <sstream>

#include <vector>

#undef ERROR
#define ERROR( X ) std::cerr << X << std::endl
#define WARNING( X ) std::cout << X << std::endl
#define INFO( X ) std::cout << X << std::endl

using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::swap;
using std::ostringstream;
using std::istringstream;


#include "gp_load_ppm.hpp"

namespace {
	Module *create_module(const std::string &params_in) {
		string params = params_in;
		char mode = 'p';
		char blend = 'a';
		if (params.size() == 2 || (params.size() > 2 && params[2] == ' ')) {
			mode = params[0];
			blend = params[1];
			if (params.size() == 2) params = "";
			else params.erase(0,3);
		}
		Vector3f grey = make_vector(0.0f, 0.0f, 0.0f);
		{
			vector< float > r,g,b;
			unsigned int width, height;
			if (load_ppm_channels(params, width, height, r, g, b)) {
				for (unsigned int i = 0; i < r.size(); ++i) {
					grey.x += r[i];
				}
				if (!r.empty()) {
					grey.x /= r.size();
				}
				for (unsigned int i = 0; i < g.size(); ++i) {
					grey.y += g[i];
				}
				if (!g.empty()) {
					grey.y /= g.size();
				}
				for (unsigned int i = 0; i < b.size(); ++i) {
					grey.z += b[i];
				}
				if (!b.empty()) {
					grey.z /= b.size();
				}
				grey -= make_vector(0.5f, 0.5f, 0.5f);
			}
		}

		GeneralPaintModule *m = new GeneralPaintModule(params, new VCycle(grey));
		if (mode == 'c') {
			m->mode = GeneralPaintModule::CLONE_MODE;
		} else if (mode == 'e') {
			m->mode = GeneralPaintModule::EDGE_MODE;
		} else {
			m->mode = GeneralPaintModule::PAINT_MODE;
		}
		string target_name = "";
		if (blend == 'o') {
			target_name = "over";
		} else if (blend == 'd') {
			target_name = "dir";
		} else if (blend == 'm') {
			target_name = "min";
		} else if (blend == 'M') {
			target_name = "max";
		} else if (blend == 'a') {
			target_name = "add";
		}
		for (unsigned int b = 0; b < m->blend_modes.size(); ++b) {
			if (m->blend_modes[b].name == target_name) {
				m->current_blend_mode = b;
				break;
			}
		}
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("generalpaint", create_module, "[[pce][odmMa]] filespec");
		}
	} fred;
}



REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
//REQUIRE_GL_EXTENSION( GL_ARB_multitexture )


#include "gp_gl_helpers.hpp"

#include "gp_hsv_helpers.hpp"

BlendMode::BlendMode(string const &_name, string const &filename) : name(_name) {
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("#define DO_X\n"
#ifdef ATI_HACK
"#define ATI_HACK\n"
#endif
, filename, GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		gx_shader = Graphics::get_program_object(frag);
		assert(gx_shader.ref);
		glUseProgramObjectARB(gx_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(gx_shader->handle, "base_gx_tex"), 0);
		glUniform1iARB(glGetUniformLocationARB(gx_shader->handle, "base_gy_tex"), 1);
		glUniform1iARB(glGetUniformLocationARB(gx_shader->handle, "brush_gx_tex"), 2);
		glUniform1iARB(glGetUniformLocationARB(gx_shader->handle, "brush_gy_tex"), 3);
		glUseProgramObjectARB(0);
	}
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("#define DO_Y\n"
#ifdef ATI_HACK
"#define ATI_HACK\n"
#endif
, filename, GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		gy_shader = Graphics::get_program_object(frag);
		assert(gy_shader.ref);
		glUseProgramObjectARB(gy_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(gy_shader->handle, "base_gx_tex"), 0);
		glUniform1iARB(glGetUniformLocationARB(gy_shader->handle, "base_gy_tex"), 1);
		glUniform1iARB(glGetUniformLocationARB(gy_shader->handle, "brush_gx_tex"), 2);
		glUniform1iARB(glGetUniformLocationARB(gy_shader->handle, "brush_gy_tex"), 3);
		glUseProgramObjectARB(0);
	}
}

GeneralPaintModule::GeneralPaintModule(string const &filename, Integrator *_integrator) : integrator(_integrator) {

	drew_recently = true;
	brush_fade = 0.0f;
	text_fade = 0.0f;

	grad_dirty = true;


	painting = false;
	edge_selecting = false;
	edge_detail = false;
	brush_size = 4;
	single_stepping = false;
	step = false;
	dump_frame = false;
	help_on = false;

	mode = PAINT_MODE;
	current_stroke = NULL;
	show_stroke_line = false;
	dragging_stroke = false;

	paint_color = make_vector(0.1f, 0.1f, 0.1f);
	paint_stroke.Step = 1.0f;
	paint_stroke.smoothing_kernel.clear();
	{ //a tad smoother.
		float sum = 0.0f;
		for (int x = -4; x <= 4; ++x) {
			float v = x / float(2.0f);
			paint_stroke.smoothing_kernel.push_back(expf(-v*v));
			sum += paint_stroke.smoothing_kernel.back();
		}
		INFO("Kernel sum is " << sum);
		for (unsigned int i = 0; i < paint_stroke.smoothing_kernel.size(); ++i) {
			paint_stroke.smoothing_kernel[i] /= sum;
		}
	}
	paint_color_selecting = false;

	example_edge_stroke.smoothing_kernel = paint_stroke.smoothing_kernel;

	clone_point = make_vector(0.0f, 0.0f);

	center_color_acc = make_vector(0.0f, 0.0f, 0.0f);
	min_color_acc = make_vector(0.0f, 0.0f, 0.0f);
	max_color_acc = make_vector(0.0f, 0.0f, 0.0f);
	color_adapt = false;
	super_scale = false;
	compare = false;

	edge_sample_tex = edge_sample_fb = 0;
	edge_perp_tex = edge_perp_fb = 0;
	edge_tan_tex = edge_tan_fb = 0;
	edge_rad = 16;
	edge_steps = 0;

	//--------------------------------------------------
	//Load an image, possibly:
	vector< float > r,g,b;
	loaded_filename = filename;
	bool have_image = load_ppm_channels(filename, ImageWidth, ImageHeight, r, g, b);
	if (!have_image) {
		loaded_filename = "blank";
		istringstream dim(filename);
		char c;
		if (dim >> ImageWidth >> c >> ImageHeight && c == 'x') {
			INFO("Interpreting filename as image dimensions.");
			if (ImageWidth * ImageHeight > 100000000) {
				INFO("Image too big, I'm not going to allocate that for you.");
				ImageWidth = ImageHeight = 1025;
			}
		} else {
			ImageWidth = ImageHeight = 257;
		}
	}

	vector< Vector3f > f(ImageWidth * ImageHeight, make_vector< float, 3 >(0));
	vector< Vector3f > gx((ImageWidth-1) * ImageHeight, make_vector< float, 3 >(0));
	vector< Vector3f > gy(ImageWidth * (ImageHeight-1), make_vector< float, 3 >(0));
	if (have_image) {
		for (unsigned int y = 0; y < ImageHeight; ++y) {
			for (unsigned int x = 0; x < ImageWidth; ++x) {
				float R = r[y * ImageWidth + x];
				float G = g[y * ImageWidth + x];
				float B = b[y * ImageWidth + x];
				/*float Y_ = Kr * R + (1 - Kr - Kb) * G + Kb * B;
				f[y * ImageWidth + x].x = Y_;
				f[y * ImageWidth + x].y = 0.5f * (B - Y_) / (1 - Kb);
				f[y * ImageWidth + x].z = 0.5f * (R - Y_) / (1 - Kr);*/
				f[y * ImageWidth + x].x = R;
				f[y * ImageWidth + x].y = G;
				f[y * ImageWidth + x].z = B;
			}
		}
		for (unsigned int y = 0; y < ImageHeight; ++y) {
			for (unsigned int x = 0; x < ImageWidth; ++x) {
				if (x + 1 < ImageWidth) {
					gx[y*(ImageWidth-1)+x] = f[y * ImageWidth + x+1] - f[y * ImageWidth + x];
				}
				if (y + 1 < ImageHeight) {
					gy[y*ImageWidth+x] = f[(y+1) * ImageWidth + x] - f[y * ImageWidth + x];
				}
			}
		}
	}
	f_orig = f;


	//-------------------------------------------------
	//Set up textures:
	
	alloc_tex(ImageWidth-1, ImageHeight, gx_tex, gx);
	alloc_fb(gx_tex, gx_fb);
	alloc_tex(ImageWidth, ImageHeight-1, gy_tex, gy);
	alloc_fb(gy_tex, gy_fb);
	alloc_tex(ImageWidth, ImageHeight, f_tex, f);
	alloc_fb(f_tex, f_fb);
	alloc_tex(ImageWidth, ImageHeight, f_orig_tex, f);

	//alloc_tex(ImageWidth-1, ImageHeight, gx_residual_tex);
	//alloc_fb(gx_residual_tex, gx_residual_fb);
	//alloc_tex(ImageWidth, ImageHeight-1, gy_residual_tex);
	//alloc_fb(gy_residual_tex, gy_residual_fb);

	alloc_tex(ImageWidth-1, ImageHeight, gx_src_tex);
	alloc_tex(ImageWidth, ImageHeight-1, gy_src_tex);

	viewable_textures.push_back(ViewableTexture(gx_tex, gx_fb, ImageWidth-1, ImageHeight, "gx"));
	viewable_textures.push_back(ViewableTexture(gy_tex, gy_fb, ImageWidth, ImageHeight-1, "gy"));
	viewable_textures.push_back(ViewableTexture(f_tex, f_fb, ImageWidth, ImageHeight, "f"));
	current_viewing_texture = viewable_textures.size() - 1;

	//--------------------------------------------------

	//set up proper (lack of) color clamping:
	if (have_ARB_color_buffer_float()) {
		glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
		Graphics::gl_errors("Color clampping");
	} else {
		WARNING("Unable to set colors unclamped [no ARB_color_buffer_float] -- you may have problems.");
	}

	mouse_pos = make_vector(0.0f, 0.0f);

	//--------------------------------------------------
	{ //interpolate:
		#ifdef ATI_HACK
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate_ATI_HACK.glsl", GL_FRAGMENT_SHADER_ARB);
		#else
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate.glsl", GL_FRAGMENT_SHADER_ARB);
		#endif
		assert(frag.ref);
		interpolate_shader = Graphics::get_program_object(frag);
		assert(interpolate_shader.ref);
		glUseProgramObjectARB(interpolate_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(interpolate_shader->handle, "small"), 0);
		glUseProgramObjectARB(0);
	}
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_scale_bias.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		scale_bias_shader = Graphics::get_program_object(frag);
		assert(scale_bias_shader.ref);
		glUseProgramObjectARB(scale_bias_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(scale_bias_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_scale_bias_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		scale_bias_subpixel_shader = Graphics::get_program_object(frag);
		assert(scale_bias_subpixel_shader.ref);
		glUseProgramObjectARB(scale_bias_subpixel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_compare.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		compare_shader = Graphics::get_program_object(frag);
		assert(compare_shader.ref);
		glUseProgramObjectARB(compare_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(compare_shader->handle, "img"), 0);
		glUniform1iARB(glGetUniformLocationARB(compare_shader->handle, "orig"), 1);
		glUseProgramObjectARB(0);
	}

	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_edge_gradient.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		edge_gradient_shader = Graphics::get_program_object(frag);
		assert(edge_gradient_shader.ref);
		glUseProgramObjectARB(edge_gradient_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(edge_gradient_shader->handle, "edge"), 0);
		glUseProgramObjectARB(0);
	}
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_sum.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		sum_shader = Graphics::get_program_object(frag);
		assert(sum_shader.ref);
		glUseProgramObjectARB(sum_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(sum_shader->handle, "tex"), 0);
		glUniform1iARB(glGetUniformLocationARB(sum_shader->handle, "base"), 1);
		glUseProgramObjectARB(0);
	}

	blend_modes.push_back(BlendMode("add", "gp_shaders/gp_blend_add.glsl"));
	blend_modes.push_back(BlendMode("dir", "gp_shaders/gp_blend_directional_add.glsl"));
	blend_modes.push_back(BlendMode("max", "gp_shaders/gp_blend_max.glsl"));
	blend_modes.push_back(BlendMode("min", "gp_shaders/gp_blend_min.glsl"));
	blend_modes.push_back(BlendMode("over", "gp_shaders/gp_blend_over.glsl"));
	current_blend_mode = 0;


	//--------------------------------------------------
	//set up depth renderbuffers for drawing.
	glGenRenderbuffersEXT(1, &gx_depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, gx_depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, ImageWidth - 1, ImageHeight);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	//Note: glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, gx_depth_rb);

	glGenRenderbuffersEXT(1, &gy_depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, gy_depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, ImageWidth, ImageHeight - 1);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	//--------------------------------------------------

	if (integrator) {
		integrator->init(this);
		integrator->gradient_changed(this);
	}

	//-------------------------------------------------
	//Set up ports:
	out_ports.push_back(&gx_port);
	out_ports.push_back(&gy_port);
	gx_port.name = "gx";
	gy_port.name = "gy";
	gx_port.position = make_vector(1.0f,  0.1f);
	gy_port.position = make_vector(1.0f, -0.1f);

	gx_port().width = ImageWidth - 1;
	gx_port().height = ImageHeight;
	gx_port().tex = gx_tex;
	gx_port().fb = gx_fb;

	gy_port().width = ImageWidth;
	gy_port().height = ImageHeight - 1;
	gy_port().tex = gy_tex;
	gy_port().fb = gy_fb;

	out_ports.push_back(&edge_sample_port);
	out_ports.push_back(&edge_tan_port);
	out_ports.push_back(&edge_perp_port);

	edge_sample_port.name = "edge-sample";
	edge_tan_port.name = "edge-tan";
	edge_perp_port.name = "edge-perp";

	edge_sample_port.position = make_vector(-0.1f, -1.0f);
	edge_tan_port.position = make_vector(0.0f, -1.0f);
	edge_perp_port.position = make_vector(0.1f, -1.0f);

	edge_tan_port().width = 0;
	edge_tan_port().height = 0;
	edge_tan_port().tex = edge_tan_tex;
	edge_tan_port().fb = edge_tan_fb;

	edge_perp_port().width = 0;
	edge_perp_port().height = 0;
	edge_perp_port().tex = edge_perp_tex;
	edge_perp_port().fb = edge_perp_fb;

	Graphics::gl_errors("init");

}

GeneralPaintModule::~GeneralPaintModule() {
}

Vector2f GeneralPaintModule::size() {
	return make_vector(2.0f, 2.0f / ImageWidth * ImageHeight);
}

void GeneralPaintModule::update(float elapsed_time) {
	//set up proper (lack of) color clamping:
	if (have_ARB_color_buffer_float()) {
		glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
		Graphics::gl_errors("Color clampping");
	}


	if (text_fade > 0.0f) {
		text_fade -= elapsed_time;
		if (text_fade < 0.0f) text_fade = 0.0f;
	}
	if (brush_fade > 0.0f) {
		brush_fade -= elapsed_time;
		if (brush_fade < 0.0f) brush_fade = 0.0f;
	}
	if (!drew_recently) return;
	if (!drew_recently) return;
	drew_recently = false;
	glDisable(GL_BLEND);
	/*
	static Uint32 last_tick = SDL_GetTicks();
	Uint32 cur_tick = SDL_GetTicks();
	const float decay = 0.5f;
	static float elapsed_avg = 0.0f;
	elapsed_avg = (elapsed_avg * decay) + (cur_tick - last_tick) / 1000.0f;
	last_tick = cur_tick;
	float norm = elapsed_avg / (1.0f / (1.0f - decay));
	static float unreported_time = 0.0f;
	unreported_time += elapsed_time;
	if (unreported_time > 1.0f) {
		unreported_time = 0.0f;
		INFO("Running " << ImageWidth << "x" << ImageHeight << " = " << (ImageWidth * ImageHeight) / 1000000.0f << " megapixel(s) at " << 1.0f / norm << " fps.");
	}*/

	if (mode == PAINT_MODE && (paint_stroke.new_points || paint_stroke.dirty)) {
		render_paint_stroke();
	}

	if (mode == EDGE_MODE && (edge_stroke.new_points || edge_stroke.dirty)) {
		render_edge_stroke();
	}

	if (mode == CLONE_MODE && (clone_stroke.new_points || clone_stroke.dirty)) {
		render_clone_stroke();
	}

	gx_port().new_flag = false;
	gy_port().new_flag = false;
	if (grad_dirty) {
		if (integrator) {
			integrator->gradient_changed(this);
		}
		gx_port().new_flag = true;
		gy_port().new_flag = true;
		grad_dirty = false;
	}

	if (integrator) {
		if (!single_stepping || step) {
			integrator->update( this );
			step = false;
		}
	}

	//-----------------------------------------

	//Sample color under cursor:
	if (color_adapt) {
		ViewableTexture const &viewing = viewable_textures.at(current_viewing_texture);
		unsigned int show_scale = 1;
		while ((show_scale + 1) * viewing.width <= ImageWidth && (show_scale + 1) * viewing.height <= ImageHeight) ++show_scale;

		float show_x_offset = 0.0f; //0.5f * ((int)(ImageWidth * draw_scale) - (int)(viewing.width * show_scale));
		float show_y_offset = 0.0f; //0.5f * ((int)(ImageHeight * draw_scale) - (int)(viewing.height * show_scale));

		int sample_x = (int)( (mouse_pos.x - show_x_offset) / (int)show_scale + 0.5f);
		int sample_y = (int)( viewing.height - (mouse_pos.y - show_y_offset) / (int)show_scale + 0.5f);
		if (sample_x < 0) sample_x = 0;
		if (sample_x >= (int)viewing.width) sample_x = viewing.width - 1;
		if (sample_y < 0) sample_y = 0;
		if (sample_y >= (int)viewing.height) sample_y = viewing.height - 1;

		bind_fb(viewing.fb, viewing.width, viewing.height);
		Vector3f sample_col;
		glReadPixels(sample_x, sample_y, 1, 1, GL_RGB, GL_FLOAT, &sample_col);
		unbind_fb();
		//INFO("Center color: " << sample_col);
		center_color_acc = center_color_acc * ColorRate + sample_col;
	}

	//keep texture ports up to date:
	gx_port().tex = gx_tex;
	gx_port().fb = gx_fb;

	gy_port().tex = gy_tex;
	gy_port().fb = gy_fb;


	glEnable(GL_BLEND);
	Graphics::gl_errors("update");
}

namespace {
	string onoff(bool val) {
		if (val) return "on";
		else return "off";
	}
};

bool GeneralPaintModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	drew_recently = true;
	//so we can handle drawing:
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_MIDDLE) {
		return false;
	}
	help.clear();

	help.push_back("F1 -- toggle this help text");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
		help_on = !help_on;
		return true;
	}

	/*
	help.push_back("ESC -- quit");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
		quit_screen(this);
		return;
	}
	*/

	help.push_back("t -- toggle current stroke line visibility (currently " + onoff(show_stroke_line) + ")");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_t) {
		show_stroke_line = !show_stroke_line;
		return true;
	}

	if (current_stroke) {
		help.push_back("g -- start dragging current stroke");
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_g) {
			dragging_stroke = true;
			drag_start = mouse_pos;
			drag_start_points = current_stroke->points;
			return true;
		}
	}

	help.push_back("LEFT/RIGHT -- change blend mode");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
		text_fade = 1.0f;
		current_blend_mode = (current_blend_mode + blend_modes.size() - 1) % blend_modes.size();
		if (current_stroke) {
			current_stroke->dirty = true;
		}
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT) {
		text_fade = 1.0f;
		current_blend_mode = (current_blend_mode + 1) % blend_modes.size();
		if (current_stroke) {
			current_stroke->dirty = true;
		}
		return true;
	}

	help.push_back("a -- toggle color adapt (currently " + onoff(color_adapt) + ")");
	if (event.type == SDL_KEYDOWN && !(event.key.keysym.mod & KMOD_SHIFT) && event.key.keysym.sym == SDLK_a) {
		color_adapt = !color_adapt;
		return true;
	}

	help.push_back("DEBUG: shift + a -- toggle 10x color scale (currently " + onoff(super_scale) + ")");
	if (event.type == SDL_KEYDOWN && (event.key.keysym.mod & KMOD_SHIFT) && event.key.keysym.sym == SDLK_a) {
		super_scale = !super_scale;
		return true;
	}

	help.push_back("backspace -- delete current stroke");
	help.push_back("SHIFT+backspace -- delete current image");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
		if (current_stroke) {
			current_stroke->clear();
			current_stroke->dirty = true;
		}
		if (event.key.keysym.mod & KMOD_SHIFT) {
			INFO("Clearing..."); //eh?
			vector< Vector3f > gx((ImageWidth-1) * ImageHeight, make_vector< float, 3 >(0));
			vector< Vector3f > gy(ImageWidth * (ImageHeight-1), make_vector< float, 3 >(0));
			for (unsigned int y = 0; y < ImageHeight; ++y) {
				for (unsigned int x = 0; x < ImageWidth; ++x) {
					if (x + 1 < ImageWidth) {
						gx[y*(ImageWidth-1)+x] = f_orig[y * ImageWidth + x+1] - f_orig[y * ImageWidth + x];
					}
					if (y + 1 < ImageHeight) {
						gy[y*ImageWidth+x] = f_orig[(y+1) * ImageWidth + x] - f_orig[y * ImageWidth + x];
					}
				}
			}
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, ImageWidth-1, ImageHeight, 0, GL_RGB, GL_FLOAT, &(gx[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_src_tex);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, ImageWidth-1, ImageHeight, 0, GL_RGB, GL_FLOAT, &(gx[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, ImageWidth, ImageHeight-1, 0, GL_RGB, GL_FLOAT, &(gy[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_src_tex);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, ImageWidth, ImageHeight-1, 0, GL_RGB, GL_FLOAT, &(gy[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			grad_dirty = true;
			Graphics::gl_errors("Clearing");
		}
		return true;
	}

	help.push_back("d -- toggle extra edge detail");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_d) {
		edge_detail = !edge_detail;
	}

	/*
	help.push_back("DEBUG: c -- toggle compare to orig (currently " + onoff(compare) + ")");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c) {
		compare = !compare;
	}

	help.push_back("DEBUG: s -- turn on/single step (currently " + onoff(single_stepping) + ")");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s) {
		single_stepping = true;
		step = true;
	}
	
	help.push_back("d -- dump screenshot");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_d && !(event.key.keysym.mod & KMOD_SHIFT)) {
		dump_frame = true;
	}
	help.push_back("SHIFT + d -- dump operator");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_d && (event.key.keysym.mod & KMOD_SHIFT)) {
		VCycle *vc = dynamic_cast< VCycle * >(integrator);
		if (vc) {
			vc->dump_operator(this);
		} else {
			ERROR("Can't dump this.");
		}
	}
	*/
	
	//select other textures to view:
	help.push_back("DEBUG: pgup/pgdn -- select other texture to view");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAGEUP) {
		current_viewing_texture = (current_viewing_texture + 1) % viewable_textures.size();
		text_fade = 1.0f;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAGEDOWN) {
		current_viewing_texture = (current_viewing_texture + viewable_textures.size() - 1) % viewable_textures.size();
		text_fade = 1.0f;
		return true;
	}

	//go to a random image:
	help.push_back("DEBUG: r -- randomize current guess");
	help.push_back("DEBUG: shift + r -- zero current guess");
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
		vector< Vector3f > f_temp(ImageWidth * ImageHeight, make_vector(0.0f, 0.0f, 0.0f));
		if (!(event.key.keysym.mod & KMOD_SHIFT)) {
			for (unsigned int i = 0; i < f_temp.size(); ++i) {
				f_temp[i].x = rand() / float(RAND_MAX);
				f_temp[i].y = rand() / float(RAND_MAX); // - 0.5f;
				f_temp[i].z = rand() / float(RAND_MAX); // - 0.5f;
			}
		}
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, ImageWidth, ImageHeight, 0, GL_RGB, GL_FLOAT, &(f_temp[0]));
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		return true;
	}

	//mode switch:
	help.push_back("up/down -- switch mode");
	if (!painting && !edge_selecting && !paint_color_selecting) {
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP) {
			text_fade = 1.0f;
			mode = (mode + 1) % NUM_MODES;
			current_stroke = NULL;
			return true;
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN) {
			text_fade = 1.0f;
			mode = (mode + NUM_MODES - 1) % NUM_MODES;
			current_stroke = NULL;
			return true;
		}
	}
	

	help.push_back("mouse wheel -- brush size");
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP) {
		brush_size -= 1;
		if (brush_size < 0.5f) brush_size = 0.5f;
		if (current_stroke) {
			current_stroke->dirty = true;
		}
		brush_fade = 1.0f;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN) {
		brush_size += 1;
		if (current_stroke) {
			current_stroke->dirty = true;
		}
		brush_fade = 1.0f;
		return true;
	}

	//"Configuration action" (different per mode)
	if (mode == PAINT_MODE) {
		help.push_back("hold right button -- color selector (hue, saturation)");
		help.push_back("hold right button + shift -- color selector (value)");
	} else if (mode == CLONE_MODE) {
		help.push_back("hold right button -- set source point");
	} else if (mode == EDGE_MODE) {
		help.push_back("hold right button -- draw over example edge");
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT ) {
		Vector2f draw_pos = mouse_pos;
		draw_pos.y = ImageHeight - draw_pos.y;
		if (dragging_stroke) {
			if (current_stroke) {
				current_stroke->points = drag_start_points;
				current_stroke->dirty = true;
			}
			dragging_stroke = false;
		}
		if (mode == PAINT_MODE) {
			paint_color_selecting = true;
		} else if (mode == CLONE_MODE) {
			clone_point = mouse_pos;
		} else if (mode == EDGE_MODE) {
			if (!painting) {
				if (SDL_GetModState() & KMOD_SHIFT) {
					example_edge_stroke.submit_point(draw_pos);
					grab_current_edge();
				} else {
					example_edge_stroke.clear();
					current_stroke = &example_edge_stroke;
					current_stroke->submit_point(draw_pos);
					edge_selecting = true;
				}
			}
		}
		brush_fade = 1.0f;
	}
	//Stop edge drawing:
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
		if (mode == PAINT_MODE) {
			paint_color_selecting = false;
		} else if (mode == EDGE_MODE) {
			if (edge_selecting) {
				edge_selecting = false;
				grab_current_edge();
			}
		}
		brush_fade = 1.0f;
	}
	//Stop Painting:
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		painting = false;
		brush_fade = 1.0f;
	}
	if (current_stroke) {
		help.push_back("shift + click left button -- line to cursor");
	}
	if (mode == PAINT_MODE) {
		help.push_back("hold left button -- paint");
	} else if (mode == CLONE_MODE) {
		help.push_back("hold left button -- clone");
	} else if (mode == EDGE_MODE) {
		help.push_back("hold left button -- draw edge copy");
	}
	//Start Painting:
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		Vector2f draw_pos = mouse_pos;
		draw_pos.y = ImageHeight - draw_pos.y;

		if (edge_selecting) {
			//do nothing.
		} else if (dragging_stroke) {
			//just put it down.
			dragging_stroke = false;
		} else if ((SDL_GetModState() & KMOD_SHIFT) && current_stroke != NULL) {
			//SHIFT -> draw line.
			current_stroke->submit_point(draw_pos);
		} else {
			//start new stroke:
			painting = true;
			//make a 'src' copy:
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gx_fb);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_src_tex);
			glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, 0, 0, ImageWidth-1, ImageHeight, 0);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gy_fb);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_src_tex);
			glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, 0, 0, ImageWidth, ImageHeight-1, 0);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			if (mode == PAINT_MODE) {
				paint_stroke.clear();
				current_stroke = &paint_stroke;
				current_stroke->submit_point(draw_pos);
			} else if (mode == CLONE_MODE) {
				clone_stroke.clear();
				current_stroke = &clone_stroke;
				current_stroke->submit_point(draw_pos);
			} else if (mode == EDGE_MODE) {
				edge_stroke.clear();
				current_stroke = &edge_stroke;
				current_stroke->submit_point(draw_pos);
			}
		}
		brush_fade = 1.0f;
	}
	//Actually paint:
	if (event.type == SDL_MOUSEMOTION) {
		//track motion 'direction':
		Vector2f old_pos = mouse_pos;
		mouse_pos.x = (local_mouse.x / size().x + 0.5f) * ImageWidth;
		mouse_pos.y = (local_mouse.y / size().y + 0.5f) * ImageHeight;

		if (dragging_stroke) {
			if (current_stroke) {
				Vector2f move = mouse_pos - drag_start;
				move.y *= -1.0;
				current_stroke->points = drag_start_points;
				for (unsigned int i = 0; i < current_stroke->points.size(); ++i) {
					current_stroke->points[i] += move;
				}
				current_stroke->dirty = true;
			} else {
				WARNING("WARNING: lost current stroke while dragging!");
				dragging_stroke = false;
			}
		} else if (paint_color_selecting) {
			Vector3f hsv = rgb_to_hsv(paint_color);
			bool do_value = (SDL_GetModState() & KMOD_SHIFT) != 0;
			Vector2f pos;
			if (do_value) {
				pos.x = (hsv.v - 0.5f) * 2.0f * ColorSelRad;
				pos.y = 0.0f;
			} else {
				pos.x = cosf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
				pos.y = sinf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
			}
			pos += mouse_pos - old_pos;
			if (do_value) {
				hsv.v = pos.x / (2.0f * ColorSelRad) + 0.5f;
				if (hsv.v < 0.0f) hsv.v = 0.0f;
				if (hsv.v > 1.0f) hsv.v = 1.0f;
			} else {
				hsv.h = atan2(pos.y, pos.x) / (2 * M_PI);
				if (hsv.h < 0.0f) hsv.h += 1.0f;
				hsv.s = length(pos) / ColorSelRad;
				if (hsv.s < 0.0f) hsv.s = 0.0f;
				if (hsv.s > 1.0f) hsv.s = 1.0f;
			}
			paint_color = hsv_to_rgb(hsv);
			if (current_stroke) {
				current_stroke->dirty = true;
			}
		} else if (painting) {
			Vector2f draw_pos = mouse_pos;
			Vector2f draw_old_pos = old_pos;
			draw_pos.y = ImageHeight - draw_pos.y;
			draw_old_pos.y = ImageHeight - draw_old_pos.y;
			if (mode == PAINT_MODE) {
				assert(current_stroke == &(paint_stroke));
				current_stroke->submit_point(draw_pos);
				//grad_line(draw_old_pos, draw_pos, brush_size, draw_dir, make_vector(1.0f, 1.0f, 1.0f, 0.5f), false);
			} else if (mode == CLONE_MODE) {
				current_stroke->submit_point(draw_pos);
				assert(current_stroke == &(clone_stroke));
				//grad_line(draw_old_pos, draw_pos, brush_size, make_vector(1.0f,1.0f), make_vector(1.0f, 1.0f, 1.0f, 1.0f), true);
			} else if (mode == EDGE_MODE) {
				assert(current_stroke == &(edge_stroke));
				current_stroke->submit_point(draw_pos);
			}
		}
		if (edge_selecting) {
			Vector2f draw_pos = mouse_pos;
			draw_pos.y = ImageHeight - draw_pos.y;
			example_edge_stroke.submit_point(draw_pos);
			if (edge_detail && example_edge_stroke.points.size() > 3) {
				grab_current_edge();
			}
		}
		brush_fade = 1.0f;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
		brush_fade = 1.0f;
		return true;
	}
	return false;
}

namespace {
void dump_texture(GLuint tex, unsigned int w, unsigned int h, float scale, float bias, string base_filename) {
	vector< Vector3f > texture(w*h);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, GL_FLOAT, &(texture[0]));
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);


	ofstream dump((base_filename + ".ppm").c_str(), std::ios::out | std::ios::binary);
	dump << "P6\n" << w << " " << h << " 255\n";
	for (unsigned int row = 0; row < h; ++row) {
		float *at = &(texture[row*w].x);
		float *end = at + w * 3;
		while (at != end) {
			int val = (int)((scale * (*at) + bias)*256);
			if (val < 0) val = 0;
			if (val > 255) val = 255;
			unsigned char v = val;
			dump.write((char *)&(v),1);
			++at;
		}
	}

	ofstream raw((base_filename + ".raw").c_str(), std::ios::out | std::ios::binary);
	raw << "RW\n" << w << " " << h << "\n";
	raw.write((char *)&(texture[0]), sizeof(float)*3*w*h);
}
} //end namespace

float GeneralPaintModule::cursor_alpha() {
	return 0.0f;
}

void GeneralPaintModule::draw(Box2f viewport, Box2f screen_viewport, float, unsigned int recurse) {
	drew_recently = true;

	assert(viewable_textures.at(2).name == "f");
	viewable_textures.at(2).tex = f_tex;
	viewable_textures.at(2).fb = f_fb;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);

	//background:
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();


	glDisable(GL_BLEND);

	//Move into a 'ImageWidthxImageHeight' screen:
	glPushMatrix();
	glTranslatef(-0.5f * size().x,-0.5f * size().y, 0.0f);
	glScalef(size().x / ImageWidth, size().y / ImageHeight, 1.0f);

	//and on with the show as per normal:

	glColor3f(1.0f, 1.0f, 1.0f);

	float show_x_offset = 0.0f;
	float show_y_offset = 0.0f;

	ViewableTexture const &viewing = viewable_textures.at(current_viewing_texture);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, viewing.tex);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	set_nearest(); //because we'll use subsample in shader.

	unsigned int show_scale = 1;
	while ((show_scale + 1) * viewing.width <= ImageWidth && (show_scale + 1) * viewing.height <= ImageHeight) ++show_scale;

	unsigned int draw_width = viewing.width * show_scale;
	unsigned int draw_height = viewing.height * show_scale;

	show_x_offset = 0.0f; //0.5f * ((int)Graphics::screen_x - (int)(viewing.width * show_scale));
	show_y_offset = 0.0f; //0.5f * ((int)Graphics::screen_y - (int)(viewing.height * show_scale));

	glPushMatrix();
	glTranslatef(show_x_offset, show_y_offset, 0.0f);


	float px_size = 0.0f;
	if (compare) {
		glUseProgramObjectARB(compare_shader->handle);
		glUniform1fARB(glGetUniformLocationARB(compare_shader->handle, "scale"), super_scale ? 10.0f : 1.0f);
		glUniform1fARB(glGetUniformLocationARB(compare_shader->handle, "bias"), 0.5f - (super_scale ? 10.0f : 1.0f) * (center_color_acc.r + center_color_acc.g + center_color_acc.b) / 3.0f * (1 - ColorRate));
	} else {
		glUseProgramObjectARB(scale_bias_subpixel_shader->handle);
		glUniform1fARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "scale"), super_scale ? 10.0f : 1.0f);
		glUniform1fARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "bias"), 0.5f - (super_scale ? 10.0f : 1.0f) * (center_color_acc.r + center_color_acc.g + center_color_acc.b) / 3.0f * (1 - ColorRate));
		px_size = 2.0f / Graphics::screen_y * viewport.size().y / screen_viewport.size().y * viewing.height / size().y;
	}

	if (compare) {
		glActiveTextureARB(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_orig_tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0-0.5f*px_size,viewing.height-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0+0.5f*px_size,viewing.height+0.5f*px_size);
	glVertex2i(0,0);

	glTexCoord2f(viewing.width-0.5f*px_size,viewing.height-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,viewing.width+0.5f*px_size,viewing.height+0.5f*px_size);
	glVertex2i(draw_width,0);

	glTexCoord2f(viewing.width-0.5f*px_size,0-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,viewing.width+0.5f*px_size,0+0.5f*px_size);
	glVertex2i(draw_width,draw_height);

	glTexCoord2f(0-0.5f*px_size,0-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0+0.5f*px_size,0+0.5f*px_size);
	glVertex2i(0,draw_height);

	glEnd();

	glUseProgramObjectARB(0);

	glPopMatrix();

	if (compare) {
		glActiveTextureARB(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}
	
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glEnable(GL_BLEND);
	float text_alpha = 1.0f;
	if (text_fade < 1.0f) text_alpha = text_fade / 1.0f;
	glColor4f(1.0f, 1.0f, 1.0f, text_alpha);

	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	//gentium->draw(viewing.name, make_vector(0.0f, ImageHeight - 41.0f), 40.0f);
	string mode_name = "";
	if (mode == PAINT_MODE) {
		mode_name = "Paint";
	} else if (mode == CLONE_MODE) {
		mode_name = "Clone";
	} else if (mode == EDGE_MODE) {
		mode_name = "Edge";
	}
	float mode_height = 0.07f / (GeneralPaintModule::scale * size().y) * ImageHeight;
	gentium->draw_right(mode_name + "/" + blend_modes[current_blend_mode].name, make_vector((float)ImageWidth, ImageHeight - mode_height), mode_height);

	float brush_alpha = 1.0f;
	if (brush_fade < 0.5f) brush_alpha = brush_fade / 0.5f;

	{ //cursor
		float w = 2.5f; //widths are multiples 'o this.

		glPushMatrix();
		glTranslatef(mouse_pos.x, mouse_pos.y, 0.0f);

		//brush outline:
		if (painting || edge_selecting) {
			glPushMatrix();
			static float amt = 0.0f;
			amt = fmodf(amt + 1.0f, 360.0f);
			glRotatef(amt, 0,0,1);
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1, 0x00ff);
			glLineWidth(w * 1.5f);
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			glBegin(GL_LINE_LOOP);
			for (unsigned int a = 0; a < 32; ++a) {
				float ang = a / float(32) * M_PI * 2.0f;
				glVertex2f(brush_size * cosf(ang), brush_size * sinf(ang));
			}
			glEnd();
			glLineWidth(w * 0.9f);
			if (painting) {
				glColor3f(1.0f, 0.0f, 0.0f);
			} else { //edge_selecting:
				glColor3f(0.0f, 0.0f, 1.0f);
			}
			glBegin(GL_LINE_LOOP);
			for (unsigned int a = 0; a < 32; ++a) {
				float ang = a / float(32) * M_PI * 2.0f;
				glVertex2f(brush_size * cosf(ang), brush_size * sinf(ang));
			}
			glEnd();
			glDisable(GL_LINE_STIPPLE);
			glPopMatrix();
		}

		//crosshair cursor:
		glLineWidth(2.5f);
		glBegin(GL_LINES);
		glColor4f(1.0f, 1.0f, 1.0f, brush_alpha);
		glVertex2f( 0.0f,-5.0f);
		glVertex2f( 0.0f, 5.0f);
		glVertex2f(-5.0f, 0.0f);
		glVertex2f( 5.0f, 0.0f);

		glColor4f(0.0f, 0.0f, 0.0f, brush_alpha);
		glVertex2f(brush_size, 0.0f);
		glVertex2f(brush_size+3, 0.0f);

		glVertex2f(-brush_size, 0.0f);
		glVertex2f(-brush_size-3, 0.0f);

		glVertex2f(0.0f, brush_size);
		glVertex2f(0.0f, brush_size+3);

		glVertex2f(0.0f,-brush_size);
		glVertex2f(0.0f,-brush_size-3);

		glColor4f(1.0f, 1.0f, 1.0f, brush_alpha);
		glVertex2f( brush_size,-3);
		glVertex2f( brush_size,+3);

		glVertex2f(-brush_size,-3);
		glVertex2f(-brush_size,+3);

		glVertex2f(-3, brush_size);
		glVertex2f( 3, brush_size);

		glVertex2f(-3,-brush_size);
		glVertex2f( 3,-brush_size);
		glEnd();

		glPopMatrix();
		glLineWidth(1.0f);
	}

	if (mode == CLONE_MODE) {
		glBegin(GL_LINES);
		glVertex2f(clone_point.x, clone_point.y - 5);
		glVertex2f(clone_point.x, clone_point.y + 5);
		glVertex2f(clone_point.x-5, clone_point.y);
		glVertex2f(clone_point.x+5, clone_point.y);

		if (painting && !clone_stroke.points.empty()) {
			Vector2f clone_offset = clone_point;
			clone_offset.y = ImageHeight - clone_offset.y;
			clone_offset -= clone_stroke.points[0];

			glVertex2f(mouse_pos.x + clone_offset.x, mouse_pos.y - clone_offset.y - 5);
			glVertex2f(mouse_pos.x + clone_offset.x, mouse_pos.y - clone_offset.y + 5);
			glVertex2f(mouse_pos.x-5 + clone_offset.x, mouse_pos.y - clone_offset.y);
			glVertex2f(mouse_pos.x+5 + clone_offset.x, mouse_pos.y - clone_offset.y);
		}
		glEnd();
	}

	if (current_stroke != NULL && show_stroke_line) {
		glPushMatrix();
		glTranslatef(0.0f, ImageHeight, 0.0f);
		glScalef(1.0f, -1.0f, 0.0f);

		glBegin(GL_LINES);
		for (unsigned int i = 0; i + 1 < current_stroke->points.size(); ++i) {
			glColor4f(i/4 % 2, i/4 % 2, i/4 % 2, brush_alpha);
			glVertex(current_stroke->points[i]);
			glVertex(current_stroke->points[i+1]);
		}
		glEnd();
		glPopMatrix();
	}

	if (mode == EDGE_MODE) {
		glPushMatrix();
		glTranslatef(0.0f, ImageHeight, 0.0f);
		glScalef(1.0f, -1.0f, 0.0f);
		if (edge_detail) {
			glBegin(GL_QUADS);
			glColor4f(0.2f, 0.2f, 0.7f, 0.5f);
			for (unsigned int i = 0; i + 1 < example_edge_stroke.points.size(); ++i ) {
				Vector2f at = example_edge_stroke.points[i];
				Vector2f next = example_edge_stroke.points[i+1];
				Vector2f along = next - at;
				Vector2f perp = brush_size * normalize(perpendicular(along));
				glVertex(at   + perp);
				glVertex(next + perp);
				glVertex(next - perp);
				glVertex(at   - perp);
			}
			glEnd();
			glLineWidth(3.0f);
			glBegin(GL_LINES);
			for (unsigned int i = 0; i + 1 < example_edge_stroke.points.size(); ++i ) {
				Vector2f at = example_edge_stroke.points[i];
				Vector2f next = example_edge_stroke.points[i+1];
				Vector2f along = next - at;
				Vector2f perp = brush_size * normalize(perpendicular(along));
				glColor4f(0.9f, 0.2f, 0.2f, 1.0f);
				glVertex(at   + perp);
				glVertex(next + perp);

				glColor4f(0.2f, 0.9f, 0.2f, 1.0f);
				glVertex(next - perp);
				glVertex(at   - perp);
			}
			glEnd();

			glBegin(GL_LINES);
			for (unsigned int i = 0; i + 1 < edge_stroke.points.size(); ++i ) {
				Vector2f at = edge_stroke.points[i];
				Vector2f next = edge_stroke.points[i+1];
				Vector2f along = next - at;
				Vector2f perp = brush_size * normalize(perpendicular(along));
				glColor4f(0.9f, 0.2f, 0.2f, 1.0f);
				glVertex(at   + perp);
				glVertex(next + perp);

				glColor4f(0.2f, 0.9f, 0.2f, 1.0f);
				glVertex(next - perp);
				glVertex(at   - perp);
			}
			glEnd();
			glLineWidth(1.0f);

		}
		glColor4f(0.9f, 0.9f, 0.9f, edge_detail?0.6f:brush_alpha);
		glBegin(GL_LINE_STRIP);
		for (unsigned int i = 0; i < example_edge_stroke.points.size(); ++i) {
			glVertex(example_edge_stroke.points[i]);
		}
		glEnd();
		glPopMatrix();
		/*
		if (edge_perp_tex != 0) {

			glUseProgramObjectARB(scale_bias_shader->handle);
			glUniform1fARB(glGetUniformLocationARB(scale_bias_shader->handle, "scale"), 10.0f);
			glUniform1fARB(glGetUniformLocationARB(scale_bias_shader->handle, "bias"), 0.5f);
			glColor3f(1.0f, 1.0f, 1.0f);

			glEnable(GL_TEXTURE_RECTANGLE_ARB);

			glPushMatrix();
			glTranslatef(10.0f, 10.0f + edge_rad, 0.0f);

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_perp_tex);
			glBegin(GL_QUADS);
			glTexCoord2f(0, edge_rad*2-1); glVertex2f(0, (float)edge_rad - 0.5f);
			glTexCoord2f(edge_steps, edge_rad*2-1); glVertex2f(edge_steps, (float)edge_rad - 0.5f);
			glTexCoord2f(edge_steps, 0); glVertex2f(edge_steps, -(float)edge_rad + 0.5f);
			glTexCoord2f(0, 0); glVertex2f(0, -(float)edge_rad + 0.5f);
			glEnd();

			glPopMatrix();

			glPushMatrix();
			glTranslatef(10.0f, 10.0f + 2 * edge_rad + 10.0f + edge_rad, 0.0f);

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_tan_tex);
			glBegin(GL_QUADS);
			glTexCoord2f(0, edge_rad*2); glVertex2f(0.5f, (float)edge_rad);
			glTexCoord2f(edge_steps-1, edge_rad*2); glVertex2f(edge_steps-0.5f, (float)edge_rad);
			glTexCoord2f(edge_steps-1, 0); glVertex2f(edge_steps-0.5f, -(float)edge_rad);
			glTexCoord2f(0, 0); glVertex2f(0.5f, -(float)edge_rad);
			glEnd();

			glPopMatrix();

			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glUseProgramObjectARB(0);

		}
		*/
	}

	//Border (so paint selector can overlap it):
	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(ImageWidth, 0.0f);
	glVertex2f(ImageWidth,  ImageHeight);
	glVertex2f(0.0f,  ImageHeight);
	glEnd();

	if (paint_color_selecting) {
		Vector3f hsv = rgb_to_hsv(paint_color);
		Vector2f pos;
		bool do_value = (SDL_GetModState() & KMOD_SHIFT) != 0;
		if (do_value) {
			pos.x = (hsv.v - 0.5f) * 2.0f * ColorSelRad;
			pos.y = 0.0f;
		} else {
			pos.x = cosf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
			pos.y = sinf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
		}
		glPushMatrix();
		glTranslatef(mouse_pos.x - pos.x, mouse_pos.y - pos.y, 0.0f);
		glScalef(ColorSelRad, ColorSelRad, 0.0f);
		float ofs = 0.02f;
		float dia = 0.04f;
		if (!do_value) {
			glPushMatrix();
			glTranslatef(ofs,-ofs, 0.0f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.0f, 0.0f);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glVertex2f(cosf(ang), sinf(ang));
			}
			glEnd();
			glBegin(GL_QUAD_STRIP);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
				glVertex2f(cosf(ang), sinf(ang));
				glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
				glVertex2f((1.0f + dia) * cosf(ang), (1.0f + dia) * sinf(ang));
			}
			glEnd();

			glPopMatrix();

			glBegin(GL_TRIANGLE_FAN);
			glColor(hsv_to_rgb(make_vector(hsv.h, 0.0f, 1.0f)));
			glVertex2f(0.0f, 0.0f);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glColor(hsv_to_rgb(make_vector(ang / (float(M_PI) * 2.0f), 1.0f, 1.0f)));
				glVertex2f(cosf(ang), sinf(ang));
			}
			glEnd();
		}
		if (do_value) {
			glPushMatrix();
			glTranslatef(ofs,-ofs, 0.0f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glBegin(GL_QUADS);
			glVertex2f(-1.0f, -0.1f);
			glVertex2f(-1.0f,  0.1f);
			glVertex2f( 1.0f,  0.1f);
			glVertex2f( 1.0f, -0.1f);
			glEnd();
			glBegin(GL_QUAD_STRIP);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia, -0.1f-dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f,  0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia,  0.1f+dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f( 1.0f,  0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f( 1.0f+dia,  0.1f+dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f( 1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f( 1.0f+dia, -0.1f-dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia, -0.1f-dia);
			glEnd();
			glPopMatrix();
			glBegin(GL_QUADS);
			glColor(hsv_to_rgb(make_vector(hsv.h, hsv.s, 0.0f)));
			glVertex2f(-1.0f, -0.1f);
			glVertex2f(-1.0f,  0.1f);
			glColor(hsv_to_rgb(make_vector(hsv.h, hsv.s, 1.0f)));
			glVertex2f( 1.0f,  0.1f);
			glVertex2f( 1.0f, -0.1f);
			glEnd();
		}
		glScalef(1.0f / ColorSelRad, 1.0f / ColorSelRad, 0.0f);
		glBegin(GL_LINES);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(pos.x - 3.0f, pos.y);
		glVertex2f(pos.x + 3.0f, pos.y);
		glVertex2f(pos.x, pos.y - 3.0f);
		glVertex2f(pos.x, pos.y + 3.0f);
		glEnd();
		glPopMatrix();
	}

	if (help_on) {
		float Size = 0.06f / (size().y * GeneralPaintModule::scale) * ImageHeight;
		float pos = ImageWidth - 40.0f - Size * 1.1f;
		for (unsigned int l = 0; l < help.size(); ++l) {
			glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
			gentium->draw(help[l], make_vector(10.0f+2.0f, pos-2.0f), Size);
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			gentium->draw(help[l], make_vector(10.0f, pos), Size);
			pos -= Size;
		}
	}

	if (dump_frame) {
		dump_frame = false;
		//a) find a free dump filename
		unsigned int number = 0;
		string filename = "";
		string filename_clean = "";
		while (1) {
			ostringstream fn;
			if (number < 1000) fn << '0';
			if (number < 100) fn << '0';
			if (number < 10) fn << '0';
			fn << number;
			filename = "dump/dump" + fn.str() + ".ppm";
			filename_clean = "dump/dump" + fn.str() + ".clean";
			ifstream test(filename.c_str());
			ifstream test2((filename_clean + ".ppm").c_str());
			char t;
			if (!(test >> t) && !(test2 >> t)) {
				break;
			}
			++number;
		}
		INFO("Dumpping to " << filename_clean << "'");
		//c) write (clean) texture:
		{
			unsigned int w = viewable_textures.at(current_viewing_texture).width;
			unsigned int h = viewable_textures.at(current_viewing_texture).height;
			float scale = super_scale ? 10.0f : 1.0f;
			float bias = 0.5f - (super_scale ? 10.0f : 1.0f) * (center_color_acc.r + center_color_acc.g + center_color_acc.b) / 3.0f * (1 - ColorRate);
			dump_texture(viewable_textures.at(current_viewing_texture).tex, w, h, scale, bias, filename_clean);

			dump_texture(gx_tex, ImageWidth-1, ImageHeight, 1.0f, 0.5f, filename_clean + ".gx");
			dump_texture(gy_tex, ImageWidth, ImageHeight-1, 1.0f, 0.5f, filename_clean + ".gy");

		}
	}

	//escape this crazy world:
	glPopMatrix();
	glEnable(GL_BLEND);


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);

	Graphics::gl_errors("draw");
}


void GeneralPaintModule::render_paint_stroke() {

	GLuint stroke_tex = 0;
	Vector3f other_paint_color = 2.0f * ( paint_color - make_vector(0.5f, 0.5f, 0.5f) );
	vector< Vector4f > stroke_profile(3, make_vector(other_paint_color, 0.0f));
	stroke_profile[0].a = 0.0f;
	stroke_profile[1].a = 1.0f / brush_size;
	stroke_profile[2].a = 0.0f;
	glGenTextures(1, &stroke_tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, stroke_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, 1, stroke_profile.size(), 0, GL_RGBA, GL_FLOAT, &(stroke_profile[0]));
	set_clamp_to_edge();
	#ifdef ATI_HACK
	set_nearest();
	#else
	set_linear(); //no ATI_HACK
	#endif
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	GLuint stroke_tan_tex = 0;
	glGenTextures(1, &stroke_tan_tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, stroke_tan_tex);
	vector< Vector4f > tan_prof(stroke_profile.size()+1, make_vector(0.0f, 0.0f, 0.0f, 0.0f));
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, 1, tan_prof.size(), 0, GL_RGBA, GL_FLOAT, &(tan_prof[0]));

	render_stroke(paint_stroke, stroke_tan_tex, stroke_tex, (stroke_profile.size() + 1) / 2, 1, brush_size * float(stroke_profile.size()) / float(stroke_profile.size()-1), false);

	glDeleteTextures(1, &stroke_tex);
	glDeleteTextures(1, &stroke_tan_tex);

	paint_stroke.new_points = 0;
	paint_stroke.dirty = false;
}

void GeneralPaintModule::render_edge_stroke() {
	render_stroke(edge_stroke, edge_tan_tex, edge_perp_tex, edge_rad, edge_steps, brush_size, false);
	edge_stroke.new_points = 0;
	edge_stroke.dirty = false;
}

void GeneralPaintModule::render_clone_stroke() {
	render_stroke(clone_stroke, gx_src_tex, gy_src_tex, 0, 0, brush_size, true);
	clone_stroke.new_points = 0;
	clone_stroke.dirty = false;
}

void GeneralPaintModule::render_stroke(Stroke const &stroke, GLuint tan_tex, GLuint perp_tex, unsigned int tex_radius, unsigned int steps_along, float brush_radius, bool clone_orient) {
	glDisable(GL_BLEND); //just in case.

	for (unsigned int dim = 0; dim < 2; ++dim) {
		if (dim == 0) {
			bind_fb(gx_fb, ImageWidth-1, ImageHeight);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, gx_depth_rb);
		} else {
			bind_fb(gy_fb, ImageWidth, ImageHeight-1);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, gy_depth_rb);
		}

		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		{ //clear possible previous version of stroke:
			glDepthFunc(GL_ALWAYS);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			unsigned int w;
			unsigned int h;
			if (dim == 0) {
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_src_tex);
				w = ImageWidth - 1;
				h = ImageHeight;
			} else {
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_src_tex);
				w = ImageWidth;
				h = ImageHeight - 1;
			}
			glUseProgramObjectARB(interpolate_shader->handle);
			glColor3f(1.0f, 1.0f, 1.0f);
			glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(0,0, 1.0f);
			glTexCoord2f(w,0); glVertex3f(w,0, 1.0f);
			glTexCoord2f(w,h); glVertex3f(w,h, 1.0f);
			glTexCoord2f(0,h); glVertex3f(0,h, 1.0f);
			glEnd();
			glUseProgramObjectARB(0);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glDepthFunc(GL_LESS);
		}
		//draw stroke:
		if (stroke.points.size() > 1) {

			glPushMatrix();
			if (dim == 0) {
				glTranslatef(-0.5f, 0.0f, 0.0f);
			} else {
				glTranslatef(0.0f,-0.5f, 0.0f);
			}

			const float DMIN = 0.1f;
			const float DMAX = 0.9f;

			//set up all the textures for blend shader:
			glActiveTextureARB(GL_TEXTURE0);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_src_tex);
			if (dim == 1) {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif
			} else set_nearest();
			//
			glActiveTextureARB(GL_TEXTURE1);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_src_tex);
			if (dim == 0) {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif

			} else set_nearest();
			//
			glActiveTextureARB(GL_TEXTURE2);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tan_tex);
			if (clone_orient) {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif
				set_clamp_to_black();
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			} else {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif
				set_clamp_to_black();
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			}
			//
			glActiveTextureARB(GL_TEXTURE3);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, perp_tex);
			if (clone_orient) {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif
				set_clamp_to_black();
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			} else {
				#ifdef ATI_HACK
				set_nearest();
				#else
				set_linear(); //no ATI_HACK
				#endif
				set_clamp_to_black();
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			}
			//
			glActiveTextureARB(GL_TEXTURE0);

			if (dim == 0) {
				glUseProgramObjectARB(blend_modes[current_blend_mode].gx_shader->handle);
			} else {
				glUseProgramObjectARB(blend_modes[current_blend_mode].gy_shader->handle);
			}

			glBegin(GL_QUADS);

			Vector2f prev = stroke.points[0];
			Vector2f cur = stroke.points[1];
			Vector2f prev_perp = perpendicular(normalize(cur - prev));

			float along = 0.0f;

			Vector2f clone_offset = make_vector(0.0f, 0.0f);
			if (clone_orient && !stroke.points.empty()) {
				clone_offset = clone_point;
				clone_offset.y = ImageHeight - clone_offset.y;
				clone_offset -= stroke.points[0];
			}

			for (unsigned int i = 2; i < stroke.points.size(); ++i) {

				Vector2f next = stroke.points[i];
				Vector2f next_perp = perpendicular(normalize(next - cur));
				Vector2f cur_perp = normalize(next_perp + prev_perp);

				float len = length(next - cur);

				if (steps_along <= 1) {
					along = 0.5f; len = 0.0f;
				}

				#define VERT(P,D) do { \
					if (clone_orient) { \
					glMultiTexCoord2fARB(GL_TEXTURE2, clone_offset.x + (P).x - 0.5f, clone_offset.y + (P).y); \
					glMultiTexCoord2fARB(GL_TEXTURE3, clone_offset.x + (P).x, clone_offset.y + (P).y - 0.5f); \
					} \
					glMultiTexCoord2fARB(GL_TEXTURE0, (P).x - 0.5f, (P).y); \
					glMultiTexCoord2fARB(GL_TEXTURE1, (P).x, (P).y - 0.5f); \
					glVertex((P), (D)); \
					} while(0)
				//give texture position:
				#define TEX(X,Y) do { \
					if (!clone_orient) { \
					glMultiTexCoord2fARB(GL_TEXTURE2, (X) - 0.5f, (Y)); \
					glMultiTexCoord2fARB(GL_TEXTURE3, (X), (Y) - 0.5f); \
					} \
					} while(0)

				//twixt-segment blob:
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				if (clone_orient) {
					glMultiTexCoord2fARB(GL_TEXTURE4, 1, 0);
				} else {
					glMultiTexCoord2fARB(GL_TEXTURE4, cur_perp.y,-cur_perp.x);
				}
				//
				TEX(along, tex_radius * 2.0f);
				VERT(cur+next_perp * brush_radius, DMAX);
				VERT(cur+prev_perp * brush_radius, DMAX);
				TEX(along, tex_radius);
				VERT(cur, DMIN);
				VERT(cur, DMIN);
				//
				TEX(along, 0.0f);
				VERT(cur-next_perp * brush_radius, DMAX);
				VERT(cur-prev_perp * brush_radius, DMAX);
				TEX(along, tex_radius);
				VERT(cur, DMIN);
				VERT(cur, DMIN);


				//segment:
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				if (clone_orient) {
					glMultiTexCoord2fARB(GL_TEXTURE4, 1, 0);
				} else {
					glMultiTexCoord2fARB(GL_TEXTURE4, next_perp.y,-next_perp.x);
				}
				//
				TEX(along, 2 * tex_radius);
				VERT(cur+next_perp * brush_radius, DMAX);
				TEX(along+len, 2 * tex_radius);
				VERT(next+next_perp * brush_radius, DMAX);
				TEX(along+len, tex_radius);
				VERT(next, DMIN);
				TEX(along, tex_radius);
				VERT(cur, DMIN);
				//
				TEX(along, 0.0f);
				VERT(cur-next_perp * brush_radius, DMAX);
				TEX(along+len, 0.0f);
				VERT(next-next_perp * brush_radius, DMAX);
				TEX(along+len, tex_radius);
				VERT(next, DMIN);
				TEX(along, tex_radius);
				VERT(cur, DMIN);

				#undef VERT
				#undef TEX

				along = along + len;
				if (along + len >= steps_along) {
					along -= steps_along;
				}
				prev = cur;
				cur = next;
				prev_perp = next_perp;
			}
			glEnd();

			glActiveTextureARB(GL_TEXTURE3);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			//
			glActiveTextureARB(GL_TEXTURE2);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			//
			glActiveTextureARB(GL_TEXTURE1);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			//
			glActiveTextureARB(GL_TEXTURE0);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			glUseProgramObjectARB(0);

			glPopMatrix();
		}

		glDisable(GL_DEPTH_TEST);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
		unbind_fb();

	} //for(dim)

	grad_dirty = true;
}


void GeneralPaintModule::grab_current_edge() {
	glDisable(GL_BLEND);
/*	//Deallocate edge texture & framebuffer:
	if (edge_perp_fb != 0) {
		glDeleteFramebuffersEXT(1, &edge_perp_fb);
		edge_perp_fb = 0;
	}
	if (edge_tan_fb != 0) {
		glDeleteFramebuffersEXT(1, &edge_tan_fb);
		edge_tan_fb = 0;
	}
	if (edge_perp_tex != 0) {
		glDeleteTextures(1, &edge_perp_tex);
		edge_perp_tex = 0;
	}
	if (edge_tan_tex != 0) {
		glDeleteTextures(1, &edge_tan_tex);
		edge_tan_tex = 0;
	}*/
	if (example_edge_stroke.points.size() <= 1) return;

	edge_rad = (int)brush_size;
	if (edge_rad < 2) edge_rad = 2;
	if (edge_rad < brush_size) edge_rad += 1;
	assert(edge_rad >= brush_size);

	edge_steps = example_edge_stroke.points.size() - 1;
	if (!edge_sample_tex) {
		alloc_tex(edge_steps, 2 * edge_rad, edge_sample_tex, vector< Vector3f >());
		alloc_fb(edge_sample_tex, edge_sample_fb);
	} else {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_sample_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, edge_steps, 2*edge_rad, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}

	bind_fb(edge_sample_fb, edge_steps, 2 * edge_rad);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	set_clamp_to_edge();
	#ifdef ATI_HACK
	set_nearest();
	#else
	set_linear(); //no ATI_HACK
	#endif

	glUseProgramObjectARB(interpolate_shader->handle);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	for (unsigned int i = 0; i + 1 < example_edge_stroke.points.size(); ++i ) {
		Vector2f at = example_edge_stroke.points[i];
		Vector2f next = example_edge_stroke.points[i+1];
		Vector2f along = next - at;
		Vector2f perp = edge_rad * normalize(perpendicular(along));
		glTexCoord(at   + perp); glVertex2f(i,   2 * edge_rad);
		glTexCoord(next + perp); glVertex2f(i+1, 2 * edge_rad);
		glTexCoord(next - perp); glVertex2f(i+1, 0);
		glTexCoord(at   - perp); glVertex2f(i,   0);
	}
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	set_nearest();
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	//Idea:
	//Take the edge-perpendicular gradient. The end.
	if (!edge_perp_tex) {
		alloc_tex(edge_steps, 2 * edge_rad - 1, edge_perp_tex, vector< Vector3f >(), true);
		alloc_fb(edge_perp_tex, edge_perp_fb);
	} else {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_perp_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, edge_steps, 2*edge_rad-1, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	if (!edge_tan_tex) {
		alloc_tex(edge_steps-1, 2 * edge_rad, edge_tan_tex, vector< Vector3f >(), true);
		alloc_fb(edge_tan_tex, edge_tan_fb);
	} else {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_tan_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, edge_steps-1, 2*edge_rad, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}

	edge_perp_port().width = edge_steps;
	edge_perp_port().height = 2 * edge_rad-1;
	edge_perp_port().tex = edge_perp_tex;
	edge_perp_port().fb = edge_perp_fb;
	edge_perp_port().new_flag = true;

	edge_tan_port().width = edge_steps-1;
	edge_tan_port().height = 2 * edge_rad;
	edge_tan_port().tex = edge_tan_tex;
	edge_tan_port().fb = edge_tan_fb;
	edge_tan_port().new_flag = true;

	edge_sample_port().width = edge_steps;
	edge_sample_port().height = 2 * edge_rad;
	edge_sample_port().tex = edge_sample_tex;
	edge_sample_port().fb = edge_sample_fb;
	edge_sample_port().new_flag = true;

	//gradient -> acc_tex

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_sample_tex);
	set_clamp_to_edge(); set_nearest();

	glUseProgramObjectARB(edge_gradient_shader->handle);

	bind_fb(edge_perp_fb, edge_steps, 2 * edge_rad - 1);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 0.0f);
	glVertex2f(0, 0);

	glTexCoord2f(edge_steps, 1.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, edge_steps, 0.0f);
	glVertex2f(edge_steps, 0);

	glTexCoord2f(edge_steps, edge_rad * 2.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, edge_steps, edge_rad * 2.0f - 1.0f);
	glVertex2f(edge_steps, edge_rad * 2.0f - 1.0f);

	glTexCoord2f(0.0f, edge_rad * 2.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, edge_rad * 2.0f - 1.0f);
	glVertex2f(0.0f, edge_rad * 2.0f - 1.0f);
	glEnd();

	unbind_fb();

	bind_fb(edge_tan_fb, edge_steps - 1, 2 * edge_rad);

	glBegin(GL_QUADS);
	glTexCoord2f(1.0f, 0.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 0.0f);
	glVertex2f(0, 0);

	glTexCoord2f(edge_steps, 0.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, edge_steps-1, 0.0f);
	glVertex2f(edge_steps-1, 0);

	glTexCoord2f(edge_steps, edge_rad*2.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, edge_steps-1, edge_rad*2.0f);
	glVertex2f(edge_steps-1, edge_rad*2.0f);

	glTexCoord2f(1.0f, edge_rad*2.0f);
	glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, edge_rad*2.0f);
	glVertex2f(0.0f, edge_rad*2.0f);
	glEnd();

	unbind_fb();


	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);


	//throw the accumulated edge info over into 'edge_tex'..
/*
	glDeleteFramebuffersEXT(1, &edge_sample_fb);
	glDeleteTextures(1, &edge_sample_tex);
*/

	Graphics::gl_errors("Update edge.");
}
