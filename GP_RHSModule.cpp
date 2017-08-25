#include "GP_RHSModule.hpp"

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
		return new RHSModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("rhs", create_module, "");
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
Graphics::ProgramObjectRef rhs_shader = NULL;
}

RHSModule::RHSModule() {
	in_ports.push_back(&gx_in);
	in_ports.push_back(&gy_in);
	out_ports.push_back(&rhs_out);
	gx_in.name = "gx";
	gy_in.name = "gy";
	rhs_out.name = "rhs";

	gx_in.position = make_vector(-0.2f, 0.1f);
	gy_in.position = make_vector(-0.2f,-0.1f);
	rhs_out.position = make_vector(0.2f, 0.0f);

	if (!rhs_shader.ref)
	{ //rhs from gx,gy:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/mg_rhs.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		rhs_shader = Graphics::get_program_object(frag);
		assert(rhs_shader.ref);
		glUseProgramObjectARB(rhs_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gx"), 0);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gy"), 1);
		glUseProgramObjectARB(0);
	}

}


Vector2f RHSModule::size() {
	return make_vector(0.4f, 0.4f);
}

void RHSModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.5f, 0.7f, 0.5f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glColor3f(0.7f, 0.5f, 0.7f);
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
	Graphics::gl_errors("RHS::draw");

}

void RHSModule::update(float elapsed_time) {
	rhs_out().new_flag = false;
	if (gx_in().width + 1 != rhs_out().width || gx_in().height != rhs_out().height) {
		rhs_out().dealloc();
		if (gx_in().width != 0 && gx_in().height != 0) {
			rhs_out().alloc(gx_in().width + 1, gx_in().height);
		}
		rhs_out().new_flag = true;
	}
	if (gx_in().new_flag || gy_in().new_flag) {
		if (gx_in().width + 1 != gy_in().width || gx_in().height != gy_in().height + 1) {
			WARNING("WARNING: have bad gy_in -- " << gx_in().width << "x" << gx_in().height << " versus " << gy_in().width << "x" << gy_in().height);
		}
		//run iteration:
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,rhs_out().width, rhs_out().height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rhs_out().fb);

		glUseProgramObjectARB(rhs_shader->handle);

		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_in().tex);
		set_clamp_to_black(); set_nearest();

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_in().tex);
		set_clamp_to_black(); set_nearest();

		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		unsigned int w = rhs_out().width;
		unsigned int h = rhs_out().height;
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

bool RHSModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
