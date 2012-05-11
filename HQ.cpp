#include "HQ.hpp"
#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Vector/Vector.hpp>
#include <Graphics/GLSLShader.hpp>

#include <cassert>
#include <iostream>
#include <vector>

REQUEST_GL_EXTENSION(GL_EXT_framebuffer_object);
REQUEST_GL_EXTENSION(GL_EXT_packed_depth_stencil);
REQUEST_GL_EXTENSION(GL_ARB_texture_non_power_of_two);
REQUEST_GL_EXTENSION(GL_ARB_texture_float);
REQUEST_GL_EXTENSION(GL_ARB_texture_rectangle);
REQUEST_GL_EXTENSION(GL_EXT_framebuffer_multisample);
REQUEST_GL_EXTENSION(GL_EXT_framebuffer_blit);
REQUEST_GL_EXTENSION(GL_ARB_shader_objects);

using std::cout;
using std::endl;

using std::vector;

unsigned int hq_bits = 8;

GLuint hq_multisample_color = 0;
GLuint hq_multisample_depth_stencil = 0;
GLuint hq_multisample_framebuffer = 0;

GLuint hq_tex = 0;
GLuint hq_framebuffer = 0;

const Vector2ui HqStaticSize = make_vector(256U, 256U);
GLuint hq_static_tex = 0;

void init_hq(unsigned int samples) {
	assert(have_EXT_framebuffer_object());
	assert(have_EXT_packed_depth_stencil());
	assert(have_ARB_texture_non_power_of_two());
	assert(have_ARB_texture_float());
	assert(have_ARB_texture_rectangle());
	assert(have_EXT_framebuffer_multisample());
	assert(have_EXT_framebuffer_blit());

	//Renderbuffer for multisample color:
	glGenRenderbuffersEXT(1, &hq_multisample_color);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, hq_multisample_color);
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_RGB16F_ARB, Graphics::screen_x, Graphics::screen_y);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	Graphics::gl_errors("hq multisample color setup");

	//Renderbuffer for depth + stencil:
	glGenRenderbuffersEXT(1, &hq_multisample_depth_stencil);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, hq_multisample_depth_stencil);
	glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, GL_DEPTH_STENCIL_EXT, Graphics::screen_x, Graphics::screen_y);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

	Graphics::gl_errors("hq multisample depth stencil setup");

	glGenFramebuffersEXT(1, &hq_multisample_framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, hq_multisample_framebuffer);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, hq_multisample_color);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, hq_multisample_depth_stencil);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, hq_multisample_depth_stencil);

	Graphics::gl_errors("hq multisample framebuffer setup");

	{ //check:
		GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
		if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
			cout << "ERROR: hq_multisample_framebuffer not complete!" << endl;
			if (ret == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT) {
				cout << "  incomplete attachment." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT) {
				cout << "  missing attachment." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT) {
				cout << "  dimensions." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT) {
				cout << "  formats." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT) {
				cout << "  draw buffer." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT) {
				cout << "  read buffer." << endl;
			} else if (ret == GL_FRAMEBUFFER_UNSUPPORTED_EXT) {
				cout << "  unsupported." << endl;
			} else {
				cout << "  other: " << ret << endl;
			}
			exit(1);
		}
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glGenTextures(1, &hq_tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, hq_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, Graphics::screen_x, Graphics::screen_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	Graphics::gl_errors("hq tex setup");

	glGenFramebuffersEXT(1, &hq_framebuffer);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, hq_framebuffer);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, hq_tex, 0);

	Graphics::gl_errors("hq framebuffer setup");

	{ //check:
		GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
		if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
			cout << "ERROR: hq_framebuffer not complete!" << endl;
			if (ret == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT) {
				cout << "  incomplete attachment." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT) {
				cout << "  missing attachment." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT) {
				cout << "  dimensions." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT) {
				cout << "  formats." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT) {
				cout << "  draw buffer." << endl;
			} else if (ret == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT) {
				cout << "  read buffer." << endl;
			} else if (ret == GL_FRAMEBUFFER_UNSUPPORTED_EXT) {
				cout << "  unsupported." << endl;
			} else {
				cout << "  other: " << ret << endl;
			}
			exit(1);
		}
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	//Texture for static:
	glGenTextures(1, &hq_static_tex);
	glBindTexture(GL_TEXTURE_2D, hq_static_tex);
	{
		vector< uint8_t > r(HqStaticSize.x * HqStaticSize.y);
		for (unsigned int i = 0; i < r.size(); ++i) {
			r[i] = rand();
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, HqStaticSize.x, HqStaticSize.y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &r[0]);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	Graphics::gl_errors("hq static setup.");

	cout << "HQ is inited." << endl;

}

bool have_hq() {
	return(hq_multisample_framebuffer != 0);
}

void start_hq_frame() {
	if (hq_multisample_framebuffer == 0) {
		return;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, hq_multisample_framebuffer);
	glEnable(GL_MULTISAMPLE);
	//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	Graphics::gl_errors("start_hq_frame");
}

void finish_hq_frame() {
	Graphics::gl_errors("after frame, before finish_hq_frame");
	if (hq_multisample_framebuffer == 0) {
		return;
	}
	assert(have_ARB_shader_objects());
	{ //check that proper fb still bound.
		GLint fb = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fb);
		if ((GLuint)fb != hq_multisample_framebuffer) {
			cout << "ERROR: Framebuffer '" << hq_multisample_framebuffer << "' turned into '" << fb << "' during draw; disabling hq mode." << endl;
			deinit_hq();
			return;
		}
	}

	//copy blended version to hq_tex:

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, hq_framebuffer);
	glBlitFramebufferEXT(
		0, 0, Graphics::screen_x, Graphics::screen_y,
		0, 0, Graphics::screen_x, Graphics::screen_y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	{ //draw hq_tex:
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_BLEND);

		static Graphics::ProgramObjectRef static_shader = NULL;
		if (static_shader.ref == NULL) {
			Graphics::ShaderObjectRef frag = Graphics::get_shader_object("shaders/static.glsl", GL_FRAGMENT_SHADER_ARB);
			assert(frag.ref);
			static_shader = Graphics::get_program_object(frag);
			assert(static_shader.ref);
			glUseProgramObjectARB(static_shader->handle);
			glUniform2fARB(glGetUniformLocationARB(static_shader->handle, "static_mul"), 1.0f / HqStaticSize.x, 1.0f / HqStaticSize.y);
			glUniform1iARB(glGetUniformLocationARB(static_shader->handle, "hq_tex"), 0);
			glUniform1iARB(glGetUniformLocationARB(static_shader->handle, "static_tex"), 1);
			glUseProgramObjectARB(0);
		}

		glUseProgramObjectARB(static_shader->handle);
		glUniform2fARB(glGetUniformLocationARB(static_shader->handle, "static_add"), rand() / float(RAND_MAX), rand() / float(RAND_MAX));
		glUniform1fARB(glGetUniformLocationARB(static_shader->handle, "scale"), 1.0f / powf(2.0f, (int)hq_bits - 1));

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, hq_static_tex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, hq_tex);

		glBegin(GL_QUADS);
		glColor3f(0.5f, 1.0f, 0.5f);
		glTexCoord2f( 0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f( 0.0f, Graphics::screen_y); glVertex2f(-1.0f,  1.0f);
		glTexCoord2f( Graphics::screen_x, Graphics::screen_y); glVertex2f( 1.0f,  1.0f);
		glTexCoord2f( Graphics::screen_x, 0.0f); glVertex2f( 1.0f, -1.0f);
		glEnd();
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);
	}
	Graphics::gl_errors("finish_hq_frame");
}

void deinit_hq() {
	if (hq_multisample_framebuffer == 0) {
		return;
	}

	if (hq_multisample_framebuffer != 0) {
		glDeleteFramebuffersEXT(1, &hq_multisample_framebuffer);
		hq_multisample_framebuffer = 0;
	}
	if (hq_multisample_color != 0) {
		glDeleteRenderbuffersEXT(1, &hq_multisample_color);
		hq_multisample_color = 0;
	}
	if (hq_multisample_depth_stencil != 0) {
		glDeleteRenderbuffersEXT(1, &hq_multisample_depth_stencil);
		hq_multisample_depth_stencil = 0;
	}

	if (hq_framebuffer != 0) {
		glDeleteFramebuffersEXT(1, &hq_framebuffer);
		hq_framebuffer = 0;
	}
	if (hq_tex != 0) {
		glDeleteTextures(1, &hq_tex);
		hq_tex = 0;
	}

	if (hq_static_tex != 0) {
		glDeleteTextures(1, &hq_static_tex);
		hq_static_tex = 0;
	}
}
