#include "GP_IterModule.hpp"

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
		istringstream p(params);
		unsigned int depth = 0;
		if (!(p >> depth)) depth = 0;
		if (depth > 10) {
			WARNING("Depth is bad.");
			depth = 10;
		}
		return new IterModule(depth);
	}
	class Fred {
	public:
		Fred() {
			register_module("iter", create_module, "depth");
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
Graphics::ProgramObjectRef iter_shader = NULL;
}

IterModule::IterModule(unsigned int depth) {
	in_ports.push_back(&guess_in);
	in_ports.push_back(&rhs_in);
	out_ports.push_back(&guess_out);
	guess_in.name = "guess in";
	rhs_in.name = "rhs";
	guess_out.name = "guess out";

	rhs_in.position = make_vector(-0.2f, 0.0f);
	guess_in.position = make_vector(0.0f,-0.2f);
	guess_out.position = make_vector(0.2f, 0.0f);

	do_iter = false;

	if (!iter_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_iter.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		iter_shader = Graphics::get_program_object(frag);
		assert(iter_shader.ref);
		glUseProgramObjectARB(iter_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}

	{ //coefs:
		float s = (1 << depth);
		float m = -8*s*s-4;
		float e = s*s + 2;
		float c = s*s - 1;
		m /= 3*s*s;
		e /= 3*s*s;
		c /= 3*s*s;
		corner = c;
		edge   = e;
		middle = m;
		magic_x = 0.0f; //yeah, so, whatever.
	}
}


Vector2f IterModule::size() {
	return make_vector(0.4f, 0.4f);
}

void IterModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glColor3f(0.7f, 0.7f, 0.7f);
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
	Graphics::gl_errors("Iter::draw");

}

void IterModule::update(float elapsed_time) {
	guess_out().new_flag = false;
	if (rhs_in().width != guess_out().width || rhs_in().height != guess_out().height) {
		guess_out().dealloc();
		guess_out().alloc(rhs_in().width, rhs_in().height);
		guess_out().new_flag = true;
	}
	if (do_iter) {
		INFO("Doing ITER ("  << rhs_in().width << "x" << rhs_in().height << ")"); //DEBUG
		do_iter = false;
		if (guess_in().width != rhs_in().width || guess_in().height != rhs_in().height) {
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

		glUseProgramObjectARB(iter_shader->handle);

		//Set up shader for this level:
		glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_corner"), corner);
		glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_edge"), edge);
		glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_x"), magic_x);
		glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_center_minus_x_inv"), 1.0f / (middle-magic_x));

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

		guess_out().new_flag = true;
	}
}

bool IterModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		do_iter = true;
		return true;
	}
	return false;
}
