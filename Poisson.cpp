#include "Poisson.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#include <iostream>
#include <algorithm>

using std::cout;
using std::swap;

REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);
REQUIRE_GL_EXTENSION(GL_EXT_framebuffer_object);
REQUIRE_GL_EXTENSION(GL_ARB_depth_texture);
REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);
//REQUIRE_GL_EXTENSION(GL_ARB_multitexture);

const unsigned int Size = 512;

namespace {
	Module *create(const std::string &params) {
		return new PoissonModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("poisson", create, "");
		}
	} fred;
}


namespace {
	void make_tex(unsigned int size, GLuint &into) {
		glGenTextures(1, &into);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, into);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		Graphics::gl_errors("tex setup");
	}

	void make_fb(unsigned int size, GLuint &fb, GLuint tex, GLuint rb = 0) {
		glGenFramebuffersEXT(1, &fb);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb);
		{ //check:
			GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
			if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
				cerr << "WARNING: FRAMEBUFFER not complete!" << endl;
			}
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		Graphics::gl_errors("fb setup");
	}

}

void PoissonModule::dealloc_tex() {
	if (cons_fb) {
		glDeleteFramebuffersEXT(1, &cons_fb);
		cons_fb = 0;
	}
	if (cons_tex) {
		glDeleteTextures(1, &cons_tex);
		cons_tex = 0;
	}
	if (cons_rb) {
		glDeleteRenderbuffersEXT(1, &cons_rb);
		cons_rb = 0;
	}
	if (soln_fb) {
		glDeleteFramebuffersEXT(1, &soln_fb);
		soln_fb = 0;
	}
	if (soln_tex) {
		glDeleteTextures(1, &soln_tex);
		soln_tex = 0;
	}
	if (temp_fb) {
		glDeleteFramebuffersEXT(1, &temp_fb);
		temp_fb = 0;
	}
	if (temp_tex) {
		glDeleteTextures(1, &temp_tex);
		temp_tex = 0;
	}
}

void PoissonModule::alloc_tex() {
	dealloc_tex();
	make_tex(Size, cons_tex);

	glGenRenderbuffersEXT(1, &cons_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, cons_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, Size, Size);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	Graphics::gl_errors("cons_rb setup");

	make_fb(Size, cons_fb, cons_tex, cons_rb);

	make_tex(Size, soln_tex);
	make_fb(Size, soln_fb, soln_tex);

	make_tex(Size, temp_tex);
	make_fb(Size, temp_fb, temp_tex);

	Graphics::gl_errors("alloc_tex");

	clear_tex();

}

void PoissonModule::clear_tex() {
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,Size,Size);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, temp_fb);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, soln_fb);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();

	Graphics::gl_errors("clear_tex");
}


PoissonModule::PoissonModule() {

	cons_fb = cons_tex = cons_rb = soln_fb = soln_tex = temp_fb = temp_tex = 0;

	alloc_tex();

	run = false;
	step = false;

	step_type = STEP_STANDARD;
	show = SHOW_CONS;

	out_ports.push_back(&iter_count);
	iter_count.name = "iters";
	iter_count.position = make_vector(size().x * 0.5f, size().y * -0.3f);
	iter_count() = 0;

	out_ports.push_back(&step_name);
	step_name.name = "step";
	step_name.position = make_vector(size().x * 0.5f, 0.0f);
	step_name() = "Standard";

	out_ports.push_back(&show_name);
	show_name.name = "show";
	show_name.position = make_vector(size().x * 0.5f, size().y * 0.3f);
	show_name() = "Constraint";

	out_ports.push_back(&graph);
	graph.name = "show";
	graph.position = make_vector(0.0f, size().y *-0.5f);
	graph().resize(Size, 0.0f);

	color_a = make_vector(1.0f, 0.0f, 0.0f);
	color_b = make_vector(1.0f, 0.0f, 0.0f);

}

Vector2f PoissonModule::size() {
	return make_vector(1.0f, 1.0f);
}

namespace {
Graphics::ProgramObjectRef subpixel_shader = NULL;
}


void PoissonModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	if (!subpixel_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		subpixel_shader = Graphics::get_program_object(frag);
		assert(subpixel_shader.ref);
		glUseProgramObjectARB(subpixel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(subpixel_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}


	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f sz = size();

	glDisable(GL_BLEND);
	if (show == SHOW_CONS) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);
	} else if (show == SHOW_SOLN) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, soln_tex);
	}
	float px_size = 2.0f / Graphics::screen_y * viewport.size().x / screen_viewport.size().x * Size / size().y;
	glUseProgramObjectARB(subpixel_shader->handle);
	glUniform1fARB(glGetUniformLocationARB(subpixel_shader->handle, "px_size"), px_size);

	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
	glTexCoord2f(Size, 0.0f); glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
	glTexCoord2f(Size, Size); glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
	glTexCoord2f(0.0f, Size); glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
	glEnd();
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glEnable(GL_BLEND);

	glUseProgramObjectARB(0);

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
	glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Poisson::draw");

}

void PoissonModule::draw_stroke() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.0f / size().x, 2.0f / size().y, 1.0f);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,Size,Size);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
	glEnable(GL_DEPTH_TEST);

	vector< Vector4f > verts;
	vector< Vector3f > colors;
	{
		const float In = 0.0f;
		const float Out = 0.5f;
		const unsigned int Segs = 4;
		for (unsigned int a = 0; a <= Segs; ++a) {
			float amt = a / float(Segs);
			float ang = float(M_PI) * (1.5 - amt);
			verts.push_back(make_vector(0.0f, 0.0f, In, 0.0f));
			verts.push_back(make_vector(cosf(ang), sinf(ang), Out, 0.0f));
		}
		for (unsigned int a = 0; a <= Segs; ++a) {
			float amt = a / float(Segs);
			float ang = float(M_PI) * (0.5 - amt);
			verts.push_back(make_vector(0.0f, 0.0f, In, 1.0f));
			verts.push_back(make_vector(cosf(ang), sinf(ang), Out, 1.0f));
		}
		verts.push_back(make_vector(0.0f, 0.0f, In, 0.0f));
		verts.push_back(make_vector(0.0f,-1.0f, Out, 0.0f));

	}


	//glClientEnable(GL_VERTEX_ARRAY);
	//glVertexPointer(4, GL_FLOAT, sizeof(Vector4f), &(verts[0]));
	for (unsigned int i = 0; i + 1 < stroke.size(); ++i) {
		Vector2f at = stroke[i];
		Vector2f to = stroke[i+1];
		Vector2f along = normalize(to - at);
		Vector2f perp = perpendicular(along);
		/*glPushMatrix();
		{
			float mat[16] = {
				along.x * rad, along.y * rad, 0.0f, 0.0f,
				perp.x * rad, perp.y * rad, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				to
			};
			glMultMatrixf(mat);
		}
		glDrawArrays(GL_QUAD_STRIP, 0, verts.size());
		glPopMatrix();*/
		//glColor3f(rand() / float(RAND_MAX), rand() / float(RAND_MAX), 0.5f);
		float rad = 4.0f / Size;
		Vector3f in_color = color_a;
		Vector3f out_color = color_b;
		glBegin(GL_QUAD_STRIP);
		for (unsigned int v = 0; v < verts.size(); ++v) {
			glColor((in_color - out_color) * (verts[v].y * 0.5f + 0.5f) + out_color);
			glVertex(make_vector(verts[v].x * along * rad + verts[v].y * perp * rad + verts[v].w * (to-at) + at, verts[v].z));
		}
		glEnd();
	}
	//glClientDisable(GL_VERTEX_ARRAY);


	glDisable(GL_DEPTH_TEST);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glPopAttrib();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void PoissonModule::update_graph(GLuint from_fb) {
	//Read back the graph:
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,Size,Size);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, from_fb);
	vector< Vector3f > colors(Size);
	glReadPixels(0,Size/2,Size,1, GL_RGB, GL_FLOAT, &(colors[0]));
	graph().resize(colors.size());
	for (unsigned int i = 0; i < colors.size(); ++i) {
		graph()[i] = colors[i].r;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();
}


void PoissonModule::update(float elapsed_time) {
	if (!stroke.empty()) {
		draw_stroke();
		Vector2f end = stroke.back();
		stroke.clear();
		stroke.push_back(end);

		update_graph(cons_fb);
	}

	if (step || run) {

		static Graphics::ProgramObjectRef jacobi_shader = NULL;
		if (!jacobi_shader.ref) {
			Graphics::ShaderObjectRef frag = Graphics::get_shader_object("p_shaders/jacobi.glsl", GL_FRAGMENT_SHADER_ARB);
			assert(frag.ref);
			jacobi_shader = Graphics::get_program_object(frag);
			assert(jacobi_shader.ref);
			glUseProgramObjectARB(jacobi_shader->handle);
			glUniform1iARB(glGetUniformLocationARB(jacobi_shader->handle, "value"), 0);
			glUniform1iARB(glGetUniformLocationARB(jacobi_shader->handle, "cons"), 1);
			glUseProgramObjectARB(0);
		}

		for (unsigned iter = 0; iter < 10 && (step || run); ++iter) {
			step = false;

			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);
			glActiveTextureARB(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, soln_tex);

			glPushAttrib(GL_VIEWPORT_BIT);
			glViewport(0,0,Size,Size);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, temp_fb);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glUseProgramObjectARB(jacobi_shader->handle);
			if (step_type == STEP_STANDARD) {
				glUniform1fARB(glGetUniformLocationARB(jacobi_shader->handle, "scale"), 0.0f);
			} else {
				glUniform1fARB(glGetUniformLocationARB(jacobi_shader->handle, "scale"), jacobi_scale);
				jacobi_scale *= 0.9f;
			}

			glColor3f(1.0f, 1.0f, 0.0f);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(-1.0f, -1.0f);
			glTexCoord2f(0.0f, Size);
			glVertex2f(-1.0f,  1.0f);
			glTexCoord2f(Size, Size);
			glVertex2f( 1.0f,  1.0f);
			glTexCoord2f(Size, 0.0f);
			glVertex2f( 1.0f, -1.0f);
			glEnd();

			glUseProgramObjectARB(0);

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glPopAttrib();

			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glActiveTextureARB(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			swap(temp_fb, soln_fb);
			swap(temp_tex, soln_tex);

			iter_count() = iter_count() + 1;
		}

		update_graph(soln_fb);

		glEnable(GL_BLEND);
	}
}


void PoissonModule::draw_distance() {

	jacobi_scale = 1.0f;
	iter_count() = 0;

	assert(cons_fb);
	assert(soln_fb);
	assert(temp_fb);

	static Graphics::ProgramObjectRef jfa_init_shader = NULL;
	static Graphics::ProgramObjectRef jfa_iter_shader = NULL;
	static Graphics::ProgramObjectRef jfa_final_shader = NULL;

	if (!jfa_init_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("p_shaders/jfa_init.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		jfa_init_shader = Graphics::get_program_object(frag);
		assert(jfa_init_shader.ref);
		glUseProgramObjectARB(jfa_init_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(jfa_init_shader->handle, "cons"), 0);
		glUniform1fARB(glGetUniformLocationARB(jfa_init_shader->handle, "size"), Size);
		glUseProgramObjectARB(0);
	}
	if (!jfa_iter_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("p_shaders/jfa_iter.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		jfa_iter_shader = Graphics::get_program_object(frag);
		assert(jfa_iter_shader.ref);
		glUseProgramObjectARB(jfa_iter_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(jfa_iter_shader->handle, "close"), 0);
		glUseProgramObjectARB(0);
	}
	if (!jfa_final_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("p_shaders/jfa_final.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		jfa_final_shader = Graphics::get_program_object(frag);
		assert(jfa_final_shader.ref);
		glUseProgramObjectARB(jfa_final_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(jfa_final_shader->handle, "close"), 0);
		glUniform1iARB(glGetUniformLocationARB(jfa_final_shader->handle, "cons"), 1);
		glUseProgramObjectARB(0);
	}


	//get ready to draw lots of full-screen quads:
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,Size,Size);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	{ //jfa_init part first:

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, soln_fb);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);

		glUseProgramObjectARB(jfa_init_shader->handle);

		glColor3f(1.0f, 1.0f, 0.0f);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(0.0f, Size);
		glVertex2f(-1.0f,  1.0f);
		glTexCoord2f(Size, Size);
		glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(Size, 0.0f);
		glVertex2f( 1.0f, -1.0f);
		glEnd();

		glUseProgramObjectARB(0);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	unsigned int step = 1;
	while (step > 0 && step < Size) {
		step *= 2;
	}
	assert(step != 0);

	unsigned int extra_step = 0;

	glUseProgramObjectARB(jfa_iter_shader->handle);
	while (step > 0 || extra_step > 0) { //jfa_iter part:

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, temp_fb);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, soln_tex);

		if (step > 0) {
			glUniform1fARB(glGetUniformLocationARB(jfa_iter_shader->handle, "step"), step);
		} else {
			glUniform1fARB(glGetUniformLocationARB(jfa_iter_shader->handle, "step"), extra_step);
		}

		glColor3f(1.0f, 1.0f, 0.0f);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(0.0f, Size);
		glVertex2f(-1.0f,  1.0f);
		glTexCoord2f(Size, Size);
		glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(Size, 0.0f);
		glVertex2f( 1.0f, -1.0f);
		glEnd();


		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		swap(temp_fb, soln_fb);
		swap(temp_tex, soln_tex);

		if (step > 0) {
			step /= 2;
		} else {
			extra_step /= 2;
		}
	}
	glUseProgramObjectARB(0);

	{//final bit 'o massaging:
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, temp_fb);

		glActiveTextureARB(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);
		glActiveTextureARB(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, soln_tex);

		glUseProgramObjectARB(jfa_final_shader->handle);


		glColor3f(1.0f, 1.0f, 0.0f);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(0.0f, Size);
		glVertex2f(-1.0f,  1.0f);
		glTexCoord2f(Size, Size);
		glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(Size, 0.0f);
		glVertex2f( 1.0f, -1.0f);
		glEnd();


		glActiveTextureARB(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glActiveTextureARB(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		swap(temp_fb, soln_fb);
		swap(temp_tex, soln_tex);

		glUseProgramObjectARB(0);
	}



	glPopAttrib();

}

bool PoissonModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			stroke.push_back(local_mouse);
			show = SHOW_CONS;
			show_name() = "Constraint";
			return true;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			if (!stroke.empty()) {
				stroke.push_back(local_mouse);
				draw_stroke();
				stroke.clear();
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
				glClear(GL_DEPTH_BUFFER_BIT);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

				return true;
			}
		}
	}
	if (event.type == SDL_MOUSEMOTION) {
		if (!stroke.empty() && length(stroke.back() - local_mouse) > 0.01f) {
			stroke.push_back(local_mouse);
			return true;
		}
	}
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_1) {
			color_a = make_vector(1.0f, 0.0f, 0.0f);
			if (event.key.keysym.mod & KMOD_SHIFT) {
				color_b = make_vector(1.0f, 0.0f, 0.0f);
			}
			return true;
		}
		if (event.key.keysym.sym == SDLK_2) {
			color_a = make_vector(0.1f, 0.1f, 0.9f);
			if (event.key.keysym.mod & KMOD_SHIFT) {
				color_b = make_vector(0.1f, 0.1f, 0.9f);
			}
			return true;
		}
		if (event.key.keysym.sym == SDLK_3) {
			color_a = make_vector(rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX));
			if (event.key.keysym.mod & KMOD_SHIFT) {
				color_b = make_vector(rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX));
			}
			return true;
		}
		if (event.key.keysym.sym == SDLK_s) {
			show = SHOW_SOLN;
			show_name() = "Solution";
			return true;
		}
		if (event.key.keysym.sym == SDLK_c) {
			show = SHOW_CONS;
			show_name() = "Constraint";
			return true;
		}
		if (event.key.keysym.sym == SDLK_d) {
			show = SHOW_SOLN;
			show_name() = "Distance";
			run = step = false;
			draw_distance();
			update_graph(soln_fb);
			jacobi_scale = 1.0f;
			return true;
		}
		if (event.key.keysym.sym == SDLK_RETURN) {
			if (step_type == STEP_STANDARD) {
				step_type = STEP_SCALED;
				step_name() = "Scaled";
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, soln_fb);
				glClear(GL_COLOR_BUFFER_BIT);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			} else {
				step_type = STEP_STANDARD;
				step_name() = "Standard";
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, soln_fb);
				glClear(GL_COLOR_BUFFER_BIT);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			}
			iter_count() = 0;
			step = false;
			run = false;
			show = SHOW_CONS;
			show_name() = "Constraint";
			update_graph(cons_fb);
			return true;
		}
		if (event.key.keysym.sym == SDLK_SPACE) {
			if (event.key.keysym.mod & KMOD_SHIFT) {
				run = true;
				step = false;
			} else {
				step = true;
				run = false;
			}
			show = SHOW_SOLN;
			show_name() = "Solution";
			return true;
		}
		if (event.key.keysym.sym == SDLK_BACKSPACE) {
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, soln_fb);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			iter_count() = 0;
			update_graph(cons_fb);
			return true;
		}
	}
	return false;
}
