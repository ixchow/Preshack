#include "ConformalModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>

using std::istringstream;

REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUIRE_GL_EXTENSION( GL_ARB_multitexture )

namespace {
	Module *create_conformalmodule(const std::string &params) {
		ConformalModule *c = new ConformalModule();
		istringstream p(params);
		p >> c->image >> c->src_steps() >> c->src_scale() >> c->nomap;
		return c;
	}
	class Fred {
	public:
		Fred() {
			register_module("conformal", create_conformalmodule);
		}
	} fred;
}

Vector2f ConformalModule::size() {
	return make_vector(2.0f, 2.0f);
}

void ConformalModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	const float LogSize = 2.0f;
	const unsigned int LogIntSize = 512;

	if (exp_tex == 0 || exp_tex_size != src_scale()) {
		exp_tex_size = src_scale();
		vector< Vector3f > exp_source(LogIntSize * LogIntSize);
		for (unsigned int y = 0; y < LogIntSize; ++y) {
			for (unsigned int x = 0; x < LogIntSize; ++x) {
				float len = exp((x / float(LogIntSize) - 1.0f) * logf(exp_tex_size));
				float ang = y / float(LogIntSize) * 2.0f * M_PI;
				exp_source[x + LogIntSize * y].x = len * cosf(ang);
				exp_source[x + LogIntSize * y].y = len * sinf(ang);
				exp_source[x + LogIntSize * y].z = 0.0f;
			}
		}

		if (exp_tex == 0) {
			glGenTextures(1, &exp_tex);
		}
		glBindTexture(GL_TEXTURE_2D, exp_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, LogIntSize, LogIntSize, 0, GL_RGB, GL_FLOAT, &(exp_source[0]));
		Graphics::gl_errors("Making exp_source");
	}

	Graphics::TextureRef src_texture = Graphics::get_texture(image);

	Graphics::ProgramObjectRef conformal_shader;
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("infty/conformal.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		conformal_shader = Graphics::get_program_object(frag);
		assert(conformal_shader.ref);
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glUseProgramObjectARB(conformal_shader->handle);
	glUniform1iARB(glGetUniformLocationARB(conformal_shader->handle, "src"), 0);
	glUniform1iARB(glGetUniformLocationARB(conformal_shader->handle, "log_tex"), 1);
	glUniform1iARB(glGetUniformLocationARB(conformal_shader->handle, "exp_tex"), 2);
	glUniform2fARB(glGetUniformLocationARB(conformal_shader->handle, "exp_fac"), 1.0f / logf(src_scale()), 1.0f / (2.0f * float(M_PI)));
	float src_ang = 0.0f;
	if (src_steps() != 0.0f) {
		src_ang = 2.0f * float(M_PI) / src_steps();
	}
	float src_len = logf(src_scale());

	//to lattice basis vectors in source space:
	Vector2f log_src_scale = make_vector(src_len, src_ang);
	Vector2f log_src_rot = make_vector(0.0f, 2.0f * float(M_PI));

	//src_combo maps to log_dst_rot:
	Vector2f src_combo = scale_mult() * log_src_scale + rot_mult() * log_src_rot;
	Vector2f log_dst_rot = make_vector(0.0f, 2.0f * float(M_PI));
	Vector2f log_dst_rot_inv = make_vector(log_dst_rot.x, -log_dst_rot.y) / length_squared(log_dst_rot);

	Vector2f fac;
	fac.x = log_dst_rot_inv.x * src_combo.x - log_dst_rot_inv.y * src_combo.y;
	fac.y = log_dst_rot_inv.y * src_combo.x + log_dst_rot_inv.x * src_combo.y;

	//fac * log_dst_rot == src_combo.


	if (log_tex == 0 || log_tex_fac != fac) {
		log_tex_fac = fac;
		vector< Vector3f > log_source(LogIntSize * LogIntSize);
		for (unsigned int y = 0; y < LogIntSize; ++y) {
			for (unsigned int x = 0; x < LogIntSize; ++x) {
				Vector2f p = (make_vector(x + 0.0f, y + 0.0f) / float(LogIntSize) - make_vector(0.5f, 0.5f)) * 2.0f * LogSize;
				p = make_vector(logf(length_squared(p)) / 2.0f, atan2f(p.y,p.x));
				p = make_vector(p.x * fac.x - p.y * fac.y, p.x * fac.y + p.y * fac.x);
				log_source[x + LogIntSize * y].xy = p;
				log_source[x + LogIntSize * y].z = 0.0f;
			}
		}
		if (log_tex != 0) {
			glDeleteTextures(1, &log_tex);
			log_tex = 0;
		}
		if (log_tex == 0) {
			glGenTextures(1, &log_tex);
		}
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, log_tex);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, LogIntSize, LogIntSize, 0, GL_RGB, GL_FLOAT, &(log_source[0]));
		Graphics::gl_errors("Making log_source");
	}

	
	glUniform2fARB(glGetUniformLocationARB(conformal_shader->handle, "log_src"), src_len, src_ang);
	glUniform2fARB(glGetUniformLocationARB(conformal_shader->handle, "fac"), fac.x, fac.y);

	Vector2f dst_number = make_vector(0.0f, 0.0f);

	if (fac.x != 0 || fac.y != 0) {
		Vector2f fac_inv = make_vector(fac.x, -fac.y) / length_squared(fac);
		dst_number.x = log_src_scale.x * fac_inv.x - log_src_scale.y * fac_inv.y;
		dst_number.y = log_src_scale.y * fac_inv.x + log_src_scale.x * fac_inv.y;
	}

	dst_number *= tween;

	if (!nomap) {
		dst_number = make_vector( dst_number.x * fac.x - dst_number.y * fac.y, dst_number.x * fac.y + dst_number.y * fac.x );
	}

	glUniform1fARB(glGetUniformLocationARB(conformal_shader->handle, "add_real"), dst_number.x);
	glUniform1fARB(glGetUniformLocationARB(conformal_shader->handle, "add_imag"), dst_number.y);
	glUniform1iARB(glGetUniformLocationARB(conformal_shader->handle, "nomap"), nomap);

	glActiveTextureARB(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, exp_tex);
	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, log_tex);
	glActiveTextureARB(GL_TEXTURE0);
	if (src_texture.ref) {
		glBindTexture(GL_TEXTURE_2D, src_texture->obj);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (nomap) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}

	//s only gets used in the nomap case.
	Vector2f s = make_vector(2.0f, 2.5f * float(M_PI));

	if (!nomap) {
		glBegin(GL_QUADS);
		glTexCoord2i(0,0); glVertex2f(-1.0f, -1.0f);
		glTexCoord2i(0, LogIntSize); glVertex2f(-1.0f,  1.0f);
		glTexCoord2i(LogIntSize, LogIntSize); glVertex2f( 1.0f,  1.0f);
		glTexCoord2i(LogIntSize,0); glVertex2f( 1.0f, -1.0f);
		glEnd();
	} else {
		glBegin(GL_QUADS);
		glTexCoord2f(-s.x,-s.y); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(-s.x, s.y); glVertex2f(-1.0f,  1.0f);
		glTexCoord2f(s.x, s.y); glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(s.x,-s.y); glVertex2f( 1.0f, -1.0f);
		glEnd();
	}

	glActiveTextureARB(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	if (src_texture.ref) {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glUseProgramObjectARB(0);

	if (lattice) {

		{ //set up clipping:
			double eqn1[4] = { 1, 0, 0, 1};
			double eqn2[4] = {-1, 0, 0, 1};
			double eqn3[4] = { 0, 1, 0, 1};
			double eqn4[4] = { 0,-1, 0, 1};
			glClipPlane(GL_CLIP_PLANE0, eqn1);
			glClipPlane(GL_CLIP_PLANE1, eqn2);
			glClipPlane(GL_CLIP_PLANE2, eqn3);
			glClipPlane(GL_CLIP_PLANE3, eqn4);
			glEnable(GL_CLIP_PLANE0);
			glEnable(GL_CLIP_PLANE1);
			glEnable(GL_CLIP_PLANE2);
			glEnable(GL_CLIP_PLANE3);
		}


		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1.0f/s.x, 1.0f/s.y, 1.0f);

		Vector2f fac_inv = make_vector(fac.x, -fac.y) / length_squared(fac);
		float mat[16] = {
			log_src_scale.x * fac_inv.x - log_src_scale.y * fac_inv.y, log_src_scale.x * fac_inv.y + log_src_scale.y * fac_inv.x, 0.0f, 0.0f,
			log_src_rot.x * fac_inv.x - log_src_rot.y * fac_inv.y, log_src_rot.x * fac_inv.y + log_src_rot.y * fac_inv.x, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		glMultMatrixf(mat);
		glBegin(GL_LINES);
		for (int x = -10; x < 10; ++x) {
			for (int y = -10; y < 10; ++y) {
				glVertex2i(x,y);
				glVertex2i(x,y+1);
				glVertex2i(x,y);
				glVertex2i(x+1,y);
			}
		}
		glEnd();

		glLoadIdentity();


		{//disable clipping.
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);
			glDisable(GL_CLIP_PLANE2);
			glDisable(GL_CLIP_PLANE3);
		}


	}

	glBegin(GL_LINE_LOOP);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Conformal::draw");

}

void ConformalModule::update(float elapsed_time) {
	if (animate) {
		tween = fmodf(fmodf(tween - 1.0f * elapsed_time, 1.0f) + 1.0f, 1.0f);
	}
}

bool ConformalModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		animate = !animate;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_l) {
		lattice = !lattice;
	}
	return false;
}
