#include "GI_StepModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

namespace {
	Module *create_module(const std::string &params) {
		return new StepModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("gi_step", create_module, "");
		}
	} fred;
}

REQUIRE_GL_EXTENSION( GL_ARB_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )

void TexInfo::dealloc() {
	if (tex) {
		glDeleteTextures(1, &tex);
		tex = 0;
	}
	size = make_vector(0U, 0U);
	seq += 1;
	Graphics::gl_errors("TexInfo::dealloc");
}

void TexInfo::alloc(Vector2ui _size) {
	dealloc();
	size = _size;
	if (size.x > 10000 || size.y > 10000) {
		assert(0);
	}
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);

	vector< uint32_t > blank(size.x * size.y, 0x00000000);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(blank[0]));

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	seq += 1;
	Graphics::gl_errors("TexPacket::alloc");
}

namespace {

void alloc_fb(GLuint tex, GLuint *fb) {
	assert(fb);
	glGenFramebuffers(1, fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fb);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
	Graphics::gl_errors("fb setup");
	{ //check:
		GLenum ret = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER );
		if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
			LOG_WARNING("WARNING: order FRAMEBUFFER not complete!");
		}
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	Graphics::gl_errors("alloc_fb");
}

Graphics::ProgramObjectRef downsample_shader = NULL;
Graphics::ProgramObjectRef upsample_shader = NULL;
}

StepModule::StepModule() : fb(0) {
	in_ports.push_back(&high_tex);
	in_ports.push_back(&low_tex);
	in_ports.push_back(&over_tex);
	out_ports.push_back(&tex);
	tex.name = "tex";
	high_tex.name = "high";
	low_tex.name = "low";
	over_tex.name = "over";

	tex.position = product(make_vector(0.0f, 0.0f), size());
	high_tex.position = product(make_vector(-0.5f, 0.0f), size());
	low_tex.position = product(make_vector( 0.5f, 0.0f), size());
	over_tex.position = product(make_vector(0.0f, 0.5f), size());

	if (downsample_shader.ref == NULL) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object_from_text(
			"#extension GL_ARB_texture_rectangle : enable \n"
			"uniform sampler2DRect tex; \n"
			"void main() { \n"
			"	vec2 at = gl_TexCoord[0].xy; \n"
			"	vec4 acc = vec4(0.0, 0.0, 0.0, 0.0); \n"
			"	#define AT(X, Y, W) \\\n"
			"		{ \\\n"
			"			vec4 col = texture2DRect(tex, at + vec2(X, Y)); \\\n"
			"			acc += (W) * vec4(col.xyz * col.w, col.w); \\\n"
			"		} \n"
			"	AT( 0, 0, 4.0/16.0) \n"
			"	AT(-1, 0, 2.0/16.0) \n"
			"	AT( 1, 0, 2.0/16.0) \n"
			"	AT( 0,-1, 2.0/16.0) \n"
			"	AT( 0, 1, 2.0/16.0) \n"
			"	AT(-1, 1, 1.0/16.0) \n"
			"	AT( 1, 1, 1.0/16.0) \n"
			"	AT(-1,-1, 1.0/16.0) \n"
			"	AT( 1,-1, 1.0/16.0) \n"
			"	if (acc.w > 0.0) { \n"
			"		gl_FragColor = vec4(acc.xyz / acc.w, acc.w * 1.5); \n"
			"	} else { \n"
			"		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0); \n"
			"	} \n"
			"} \n",
			GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		downsample_shader = get_program_object(frag);
		assert(downsample_shader.ref);
		glUseProgramObjectARB(downsample_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(downsample_shader->handle, "tex"), 0);
		glUseProgramObjectARB(0);
	}
	if (upsample_shader.ref == NULL) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object_from_text(
			"#extension GL_ARB_texture_rectangle : enable \n"
			"uniform sampler2DRect tex; \n"
			"uniform sampler2DRect small; \n"
			"void main() { \n"
			"	vec2 at = gl_TexCoord[0].xy; \n"
			"	vec4 under = texture2DRect(small, 0.5 * at + vec2(0.25, 0.25)); \n"
			"	vec4 over = texture2DRect(tex, at); \n"
			"	if (over.w > 0.0) { \n"
			"		vec3 num = under.xyz * under.w * (1.0 - over.w) + over.xyz * over.w; \n"
			"		num /= under.w * (1.0 - over.w) + over.w; \n"
			"		gl_FragColor = vec4(num, 1.0); \n"
			"	} else { \n"
			"		gl_FragColor = vec4(under.xyz, 1.0); \n"
			"	} \n"
			"} \n",
			GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		upsample_shader = get_program_object(frag);
		assert(upsample_shader.ref);
		glUseProgramObjectARB(upsample_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(upsample_shader->handle, "tex"), 0);
		glUniform1iARB(glGetUniformLocationARB(upsample_shader->handle, "small"), 1);
		glUseProgramObjectARB(0);
	}


}

StepModule::~StepModule() {
	tex().dealloc();
}


Vector2f StepModule::size() {
	Vector2f s = make_vector(1.0f, 1.0f);
	if (tex().size.x > 0 && tex().size.y > 0) {
		s = make_vector(tex().size.x / 64.0f, tex().size.y / 64.0f);
	}
	if (s.x < 0.5) {
		s = s / s.x * 0.5;
	}
	if (s.y < 0.5) {
		s = s / s.y * 0.5;
	}
	return s;
}

void StepModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();
	/*
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x, -0.5f*s.y);
	glVertex2f( 0.5f*s.x,  0.5f*s.y);
	glVertex2f(-0.5f*s.x,  0.5f*s.y);
	glEnd();
	*/

	if (tex().tex != 0) {
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex().tex);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0, 0);
		glVertex2f(-0.5f*s.x, -0.5f*s.y);
		glTexCoord2f(tex().size.x, 0);
		glVertex2f( 0.5f*s.x, -0.5f*s.y);
		glTexCoord2f(tex().size.x, tex().size.y);
		glVertex2f( 0.5f*s.x,  0.5f*s.y);
		glTexCoord2f(0, tex().size.y);
		glVertex2f(-0.5f*s.x,  0.5f*s.y);
		glEnd();
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
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
	Graphics::gl_errors("Step::draw");

}

void StepModule::update(float elapsed_time) {
	Vector2ui old_size = tex().size;
	if (over_tex().size.x > 0U && over_tex().size.y > 0U) {
		//we're a "scale up/paste over" kinda thing.
		if (tex().size != over_tex().size) {
			tex().alloc(over_tex().size);
		}
		assert(tex().size == over_tex().size);
		Vector2ui wanted;
		wanted.x = (tex().size.x - 1) / 2 + 1;
		wanted.y = (tex().size.y - 1) / 2 + 1;
		if (wanted != low_tex().size) {
			LOG_WARNING("Lower scale doesn't match over_tex.");
		}
	} else if (high_tex().size.x > 0U && high_tex().size.y > 0U) {
		Vector2ui wanted;
		wanted.x = (high_tex().size.x - 1) / 2 + 1;
		wanted.y = (high_tex().size.y - 1) / 2 + 1;
		if (tex().size != wanted) {
			tex().alloc(wanted);
		}
	}
	if (tex().size != old_size) {
		tex.position = product(make_vector(0.0f, 0.0f), size());
		high_tex.position = product(make_vector(-0.5f, 0.0f), size());
		low_tex.position = product(make_vector( 0.5f, 0.0f), size());
		over_tex.position = product(make_vector(0.0f, 0.5f), size());
	}
}

bool StepModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {

		if (tex().tex) {
			if (fb == 0) {
				alloc_fb(tex().tex, &fb);
			}
			glPushAttrib(GL_VIEWPORT_BIT);

			glViewport(0,0,tex().size.x, tex().size.y);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, tex().tex, 0);

			if (high_tex().tex) {
				LOG_INFO("Downsampling...");
		
				glUseProgramObjectARB(downsample_shader->handle);

				glEnable(GL_TEXTURE_RECTANGLE_ARB);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, high_tex().tex);

				glDisable(GL_BLEND);
				glBegin(GL_QUADS);
				const float Base = -0.5f;
				glTexCoord2f(Base, Base);
				glVertex2f(-1.0f, -1.0f);
				glTexCoord2f(Base + tex().size.x * 2.0, Base);
				glVertex2f( 1.0f, -1.0f);
				glTexCoord2f(Base + tex().size.x * 2.0, Base + tex().size.y * 2.0);
				glVertex2f( 1.0f,  1.0f);
				glTexCoord2f(Base, Base + tex().size.y * 2.0);
				glVertex2f(-1.0f,  1.0f);
				glEnd();
				glEnable(GL_BLEND);

				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				glDisable(GL_TEXTURE_RECTANGLE_ARB);

				glUseProgramObjectARB(0);

				Graphics::gl_errors("downsample");
			} else if (low_tex().tex && over_tex().tex) {
			
				LOG_INFO("Upsampling...");
				glUseProgramObjectARB(upsample_shader->handle);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, low_tex().tex);
				glEnable(GL_TEXTURE_RECTANGLE_ARB);

				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, over_tex().tex);
				glEnable(GL_TEXTURE_RECTANGLE_ARB);

				glDisable(GL_BLEND);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-1.0f, -1.0f);
				glTexCoord2f(tex().size.x, 0);
				glVertex2f( 1.0f, -1.0f);
				glTexCoord2f(tex().size.x, tex().size.y);
				glVertex2f( 1.0f,  1.0f);
				glTexCoord2f(0, tex().size.y);
				glVertex2f(-1.0f,  1.0f);
				glEnd();
				glEnable(GL_BLEND);

				glActiveTexture(GL_TEXTURE1);
				glDisable(GL_TEXTURE_RECTANGLE_ARB);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
				glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);

				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				glActiveTexture(GL_TEXTURE0);
				glDisable(GL_TEXTURE_RECTANGLE_ARB);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

				glUseProgramObjectARB(0);

				Graphics::gl_errors("upsample");
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			glPopAttrib();
			Graphics::gl_errors("operate");
		}


	}
	return false;
}
