#include "GP_ProjectModule.hpp"

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
		return new ProjectModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("project", create_module, "");
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
Graphics::ProgramObjectRef interpolate_shader = NULL;
}

ProjectModule::ProjectModule() {
	in_ports.push_back(&in);
	in_ports.push_back(&out);
	in.name = "in";
	out.name = "out";

	in.position = make_vector(0.0f,-0.2f);
	out.position = make_vector(0.0f, 0.2f);

	do_project = false;

	if (!interpolate_shader.ref) { //interpolate:
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

}


Vector2f ProjectModule::size() {
	return make_vector(0.4f, 0.4f);
}

void ProjectModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	glBegin(GL_QUADS);
	glColor3f(0.5f, 0.5f, 0.7f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glColor3f(0.7f, 0.5f, 0.5f);
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
	Graphics::gl_errors("Project::draw");

}

void ProjectModule::update(float elapsed_time) {
	if (do_project) {
		do_project = false;

		unsigned int targ_width = ((out().width - 1) / 2) + 1;
		unsigned int targ_height = ((out().height - 1) / 2) + 1;
		if (targ_width != in().width || targ_height != in().height) {
			WARNING("Expecting a different size for project input/output");
		}
		if (in().tex == 0 || out().fb == 0) {
			ERROR("ERROR: No output fb or perhaps input tex for project.");
			return;
		}

		//run iteration:
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,out().width, out().height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, out().fb);

		glUseProgramObjectARB(interpolate_shader->handle);

		glActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, in().tex);
		set_clamp_to_black();
		#ifdef ATI_HACK
		set_nearest();
		#else
		set_linear(); //no ATI_HACK
		#endif

		glDisable(GL_BLEND);
		glBegin(GL_QUADS);
		unsigned int w = out().width;
		unsigned int h = out().height;
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.25f,        0.25f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(0.25f+0.5f*w, 0.25f); glVertex2f( 1.0f, -1.0f);
		glTexCoord2f(0.25f+0.5f*w, 0.25f+0.5f*h); glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(0.25f,        0.25f+0.5f*h); glVertex2f(-1.0f,  1.0f);
		glEnd();
		glEnable(GL_BLEND);

		glActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glPopAttrib();
	}
}

bool ProjectModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		do_project = true;
		return true;
	}
	return false;
}
