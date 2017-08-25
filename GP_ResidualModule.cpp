#include "GP_ResidualModule.hpp"

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
	Module *create_module(string const &) {
		return new ResidualModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("residual", create_module);
		}
	} fred;
}

REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_ARB_multitexture )

#include "gp_gl_helpers.hpp"

namespace {
Graphics::ProgramObjectRef residual_shader = NULL;
}

ResidualModule::ResidualModule() {
	in_ports.push_back(&guess_in);
	in_ports.push_back(&rhs_in);
	out_ports.push_back(&rhs_out);
	guess_in.name = "guess in";
	rhs_in.name = "rhs in";
	rhs_out.name = "rhs out";

	rhs_in.position = make_vector(-0.2f, 0.0f);
	guess_in.position = make_vector(0.0f,0.2f);
	rhs_out.position = make_vector(0.2f, 0.0f);

	do_residual = false;

	if (!residual_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_residual.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		residual_shader = Graphics::get_program_object(frag);
		assert(residual_shader.ref);
		glUseProgramObjectARB(residual_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(residual_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(residual_shader->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}

	corner = 0.0f;
	edge = 1.0f;
	middle = -4.0f;
}


Vector2f ResidualModule::size() {
	return make_vector(0.4f, 0.4f);
}

void ResidualModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.9f, 0.5f, 0.5f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glColor3f(0.5f, 0.7f, 0.7f);
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
	Graphics::gl_errors("Residual::draw");

}

void ResidualModule::update(float elapsed_time) {
	rhs_out().new_flag = false;
	if (rhs_in().width != rhs_out().width || rhs_in().height != rhs_out().height) {
		rhs_out().dealloc();
		rhs_out().alloc(rhs_in().width, rhs_in().height);
		rhs_out().new_flag = true;
	}
	if (do_residual) {
		do_residual = false;
		if (guess_in().width != rhs_in().width || guess_in().height != rhs_in().height) {
			WARNING("WARNING: have bad guess_in!");
		}
		//run iteration:
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,rhs_out().width, rhs_out().height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rhs_out().fb);

		glUseProgramObjectARB(residual_shader->handle);

		//Set up shader for this level:
		glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_corner"), corner);
		glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_edge"), edge);
		glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_center"), middle);

		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rhs_in().tex);
		set_nearest();

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, guess_in().tex);
		set_clamp_to_edge(); set_nearest();

		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		unsigned int w = rhs_in().width;
		unsigned int h = rhs_in().height;
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2i(0, 0); glVertex2f(-1.0f, -1.0f);
		glTexCoord2i(w, 0); glVertex2f( 1.0f, -1.0f);
		glTexCoord2i(w, h); glVertex2f( 1.0f,  1.0f);
		glTexCoord2i(0, h); glVertex2f(-1.0f,  1.0f);
		glEnd();

		glEnable(GL_BLEND);
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glPopAttrib();

		rhs_out().new_flag = true;
	}
}

bool ResidualModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		do_residual = true;
		return true;
	}
	return false;
}
