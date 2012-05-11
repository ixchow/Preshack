#include "GP_AddModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#undef ERROR
#define ERROR( X ) std::cerr << X << std::endl
#define WARNING( X ) std::cout << X << std::endl
#define INFO( X ) std::cout << X << std::endl

#include <sstream>

using std::istringstream;

namespace {
	Module *create_module(const std::string &params) {
		return new AddModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("add", create_module);
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

namespace {
Graphics::ProgramObjectRef interpolate_add_shader = NULL;
}

AddModule::AddModule() {
	in_ports.push_back(&guess_in);
	in_ports.push_back(&correction_in);
	out_ports.push_back(&guess_out);
	guess_in.name = "guess in";
	correction_in.name = "correction";
	guess_out.name = "guess out";

	correction_in.position = make_vector(-0.2f, 0.0f);
	guess_in.position = make_vector(0.0f,-0.2f);
	guess_out.position = make_vector(0.2f, 0.0f);

	do_add = false;

	if (!interpolate_add_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate_add.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		interpolate_add_shader = Graphics::get_program_object(frag);
		assert(interpolate_add_shader.ref);
		glUseProgramObjectARB(interpolate_add_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(interpolate_add_shader->handle, "small"), 0);
		glUniform1iARB(glGetUniformLocationARB(interpolate_add_shader->handle, "add"), 1);
		glUseProgramObjectARB(0);
	}
}


Vector2f AddModule::size() {
	return make_vector(0.4f, 0.4f);
}

void AddModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glColor3f(0.5f, 0.5f, 0.7f);
	glVertex2f( 0.5f*s.x,  0.5f*s.y);
	glVertex2f(-0.5f*s.x,  0.5f*s.y);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x,  0.5f*s.y);
	glVertex2f(-0.5f*s.x,  0.5f*s.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Add::draw");

}

void AddModule::update(float elapsed_time) {
	guess_out().new_flag = false;
	if (correction_in().width != guess_out().width || correction_in().height != guess_out().height) {
		guess_out().dealloc();
		guess_out().alloc(correction_in().width, correction_in().height);
		guess_out().new_flag = true;
	}
	if (do_add) {
		do_add = false;
		if (guess_in().width != correction_in().width || guess_in().height != correction_in().height) {
			WARNING("WARNING: have bad guess_in!");
		}
		//run iteration:
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,guess_out().width, guess_out().height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, guess_out().fb);

		glUseProgramObjectARB(interpolate_add_shader->handle);

		glActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, guess_in().tex);
		set_nearest();

		glActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, correction_in().tex);
		set_clamp_to_edge(); set_nearest();

		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		unsigned int w = correction_in().width;
		unsigned int h = correction_in().height;
		unsigned int w2 = guess_in().width;
		unsigned int h2 = guess_in().height;
		glColor3f(1.0f, 1.0f, 1.0f);
		glMultiTexCoord2iARB(GL_TEXTURE1, 0, 0);
		glTexCoord2i(0, 0); glVertex2f(-1.0f, -1.0f);
		glMultiTexCoord2iARB(GL_TEXTURE1, w2, 0);
		glTexCoord2i(w, 0); glVertex2f( 1.0f, -1.0f);
		glMultiTexCoord2iARB(GL_TEXTURE1, w2, h2);
		glTexCoord2i(w, h); glVertex2f( 1.0f,  1.0f);
		glMultiTexCoord2iARB(GL_TEXTURE1, 0, h2);
		glTexCoord2i(0, h); glVertex2f(-1.0f,  1.0f);
		glEnd();

		glEnable(GL_BLEND);
		glActiveTextureARB(GL_TEXTURE1);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glPopAttrib();

		guess_out().new_flag = true;
	}
}

bool AddModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		do_add = true;
		return true;
	}
	return false;
}
