#include "GP_TexViewerModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#undef ERROR
#define ERROR( X ) std::cerr << X << std::endl
#define WARNING( X ) std::cout << X << std::endl
#define INFO( X ) std::cout << X << std::endl

namespace {
	Module *create_module(const std::string &params) {
		return new TexViewerModule(false, params == "r");
	}
	Module *create_module2(const std::string &params) {
		return new TexViewerModule(true, params == "r");
	}
	class Fred {
	public:
		Fred() {
			register_module("texviewer", create_module, "[r]");
			register_module("texcontainer", create_module2, "[r]");
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

void TexPacket::dealloc() {
	if (tex) {
		glDeleteTextures(1, &tex);
		tex = 0;
	}
	if (fb) {
		glDeleteFramebuffersEXT(1, &fb);
		fb = 0;
	}
	width = height = 0;
	Graphics::gl_errors("TexPacket::dealloc");
}

void TexPacket::alloc(unsigned int _width, unsigned int _height) {
	width = _width;
	height = _height;
	if (tex || fb) {
		WARNING("WARNING: TexPacket::alloc on non-zero tex/fb: " << tex << "/" << fb);
	}
	if (width > 10000 || height > 10000) {
		assert(0);
	}
	alloc_tex(width, height, tex);
	alloc_fb(tex, fb);
	Graphics::gl_errors("TexPacket::alloc");
}

namespace {
Graphics::ProgramObjectRef scale_bias_shader = NULL;
}

TexViewerModule::TexViewerModule( bool _container, bool _rotate) : container(_container), rotate(_rotate) {
	in_ports.push_back(&tex);
	out_ports.push_back(&tex_out);
	tex.name = "tex";
	tex_out.name = "tex out";

	tex.position = make_vector(-0.5f, 0.0f);
	tex_out.position = make_vector(0.5f, 0.0f);

	if (!scale_bias_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_scale_bias_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		scale_bias_shader = Graphics::get_program_object(frag);
		assert(scale_bias_shader.ref);
		glUseProgramObjectARB(scale_bias_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(scale_bias_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}

}


Vector2f TexViewerModule::size() {
	float h = 1.0f;
	if (tex_out().width > 0 && tex_out().height > 0) {
		if (rotate) {
			h = 1.0f / tex_out().height * tex_out().width;
		} else {
			h = 1.0f / tex_out().width * tex_out().height;
		}
	}
	return make_vector(1.0f, h);
}

void TexViewerModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x,  0.5f*s.y);
	glVertex2f(-0.5f*s.x,  0.5f*s.y);
	glEnd();

	if (tex_out().tex != 0) {

		glDisable(GL_BLEND);
		glUseProgramObjectARB(scale_bias_shader->handle);
		glUniform1fARB(glGetUniformLocationARB(scale_bias_shader->handle, "scale"), 1.0f);
		glUniform1fARB(glGetUniformLocationARB(scale_bias_shader->handle, "bias"), 0.5f);
		glColor3f(1.0f, 1.0f, 1.0f);

		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_out().tex);
		#ifdef ATI_HACK
		set_nearest();
		#else
		set_linear(); //no ATI_HACK
		#endif
		float px_size = 2.0f / Graphics::screen_y * viewport.size().x / screen_viewport.size().x * tex_out().height / size().y;
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glTexCoord2f(0-0.5f * px_size, 0-0.5f * px_size);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0+0.5f * px_size, 0+0.5f * px_size);
		if (rotate) {
			glVertex2f(-0.5f*s.x, -0.5f*s.y);
		} else {
			glVertex2f(-0.5f*s.x,  0.5f*s.y);
		}

		glTexCoord2f(tex_out().width-0.5f * px_size, 0-0.5f * px_size);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, tex_out().width+0.5f * px_size, 0+0.5f * px_size);

		if (rotate) {
			glVertex2f(-0.5f*s.x,  0.5f*s.y);
		} else {
			glVertex2f( 0.5f*s.x,  0.5f*s.y);
		}

		glTexCoord2f(tex_out().width-0.5f * px_size, tex_out().height-0.5f * px_size);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, tex_out().width+0.5f * px_size, tex_out().height+0.5f * px_size);

		if (rotate) {
			glVertex2f( 0.5f*s.x,  0.5f*s.y);
		} else {
			glVertex2f( 0.5f*s.x, -0.5f*s.y);
		}

		glTexCoord2f(0-0.5f * px_size, tex_out().height-0.5f * px_size);

		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0+0.5f * px_size, tex_out().height+0.5f * px_size);

		if (rotate) {
			glVertex2f( 0.5f*s.x, -0.5f*s.y);
		} else {
			glVertex2f(-0.5f*s.x, -0.5f*s.y);
		}
		glEnd();
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);

		glUseProgramObjectARB(0);
		glEnable(GL_BLEND);
	} else {
		glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2f(-0.4f*s.x, -0.4f*s.y);
		glVertex2f( 0.4f*s.x,  0.4f*s.y);
		glVertex2f( 0.4f*s.x, -0.4f*s.y);
		glVertex2f(-0.4f*s.x,  0.4f*s.y);
		glEnd();
	}

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x,  0.5f*s.y);
	glVertex2f(-0.5f*s.x,  0.5f*s.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("TexViewer::draw");

}

void TexViewerModule::update(float elapsed_time) {
	if (!container) {
		//just pass through the supplied texture info.
		tex_out() = tex();
	} else {
		//make a local copy of the supplied info for ourself.
		bool do_copy = false;
		if (tex().width != tex_out().width || tex().height != tex_out().height) {
			tex_out().dealloc();
			if (tex().width != 0 && tex().height != 0) {
				tex_out().alloc(tex().width, tex().height);
				do_copy = true;
			}
		}
		if (tex().new_flag) {
			do_copy = true;
		}
		if (do_copy) {
			glPushAttrib(GL_VIEWPORT_BIT);

			glViewport(0,0,tex_out().width, tex_out().height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tex_out().fb);
			glDisable(GL_BLEND);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex().tex);
			//TODO: invoke shader to prevent clamp.
			glBegin(GL_QUADS);
			unsigned int w = tex().width;
			unsigned int h = tex().height;
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexCoord2i(0, 0); glVertex2f(-1.0f, -1.0f);
			glTexCoord2i(w, 0); glVertex2f( 1.0f, -1.0f);
			glTexCoord2i(w, h); glVertex2f( 1.0f,  1.0f);
			glTexCoord2i(0, h); glVertex2f(-1.0f,  1.0f);
			glEnd();
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glEnable(GL_BLEND);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glPopAttrib();
		}
	}
}

bool TexViewerModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT && container && tex_out().fb) {
		glPushAttrib(GL_VIEWPORT_BIT);

		glViewport(0,0,tex_out().width, tex_out().height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tex_out().fb);
		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f( 1.0f, -1.0f);
		glVertex2f( 1.0f,  1.0f);
		glVertex2f(-1.0f,  1.0f);
		glEnd();
		glEnable(GL_BLEND);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glPopAttrib();
		Graphics::gl_errors("Clear tex");

	}
	return false;
}
