#include "PoissonTiming.hpp"

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
REQUIRE_GL_EXTENSION(GL_ARB_framebuffer_object);
REQUIRE_GL_EXTENSION(GL_ARB_depth_texture);
REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);
//REQUIRE_GL_EXTENSION(GL_ARB_multitexture);

const unsigned int Size = 1024;

namespace {
	Module *create(const std::string &params) {
		return new PoissonTimingModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("poisson_timing", create, "");
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
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
		{ //check:
			GLenum ret = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER );
			if (ret != GL_FRAMEBUFFER_COMPLETE) {
				cerr << "WARNING: FRAMEBUFFER not complete!" << endl;
			}
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		Graphics::gl_errors("fb setup");
	}

}

void PoissonTimingModule::dealloc_tex() {
	if (cons_fb) {
		glDeleteFramebuffers(1, &cons_fb);
		cons_fb = 0;
	}
	if (cons_tex) {
		glDeleteTextures(1, &cons_tex);
		cons_tex = 0;
	}
	if (cons_rb) {
		glDeleteRenderbuffers(1, &cons_rb);
		cons_rb = 0;
	}
	if (soln_fb) {
		glDeleteFramebuffers(1, &soln_fb);
		soln_fb = 0;
	}
	if (soln_tex) {
		glDeleteTextures(1, &soln_tex);
		soln_tex = 0;
	}
	if (temp_fb) {
		glDeleteFramebuffers(1, &temp_fb);
		temp_fb = 0;
	}
	if (temp_tex) {
		glDeleteTextures(1, &temp_tex);
		temp_tex = 0;
	}
}

void PoissonTimingModule::alloc_tex() {
	dealloc_tex();
	make_tex(Size, cons_tex);

	glGenRenderbuffers(1, &cons_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, cons_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Size, Size);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	Graphics::gl_errors("cons_rb setup");

	make_fb(Size, cons_fb, cons_tex, cons_rb);

	make_tex(Size, soln_tex);
	make_fb(Size, soln_fb, soln_tex);

	make_tex(Size, temp_tex);
	make_fb(Size, temp_fb, temp_tex);

	Graphics::gl_errors("alloc_tex");

	clear_tex();

}

void PoissonTimingModule::clear_tex() {
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,Size,Size);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temp_fb);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, soln_fb);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cons_fb);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glPopAttrib();

	Graphics::gl_errors("clear_tex");
}


PoissonTimingModule::PoissonTimingModule() {

	cons_fb = cons_tex = cons_rb = soln_fb = soln_tex = temp_fb = temp_tex = 0;

	alloc_tex();

	run = false;
	step = false;

	show = SHOW_CONS;

	out_ports.push_back(&iter_count);
	iter_count.name = "iters";
	iter_count.position = make_vector(size().x * 0.5f, size().y * -0.3f);
	iter_count() = 0;

	color_a = make_vector(1.0f, 0.0f, 0.0f);
	color_b = make_vector(1.0f, 0.0f, 0.0f);

}

Vector2f PoissonTimingModule::size() {
	return make_vector(1.0f, 1.0f);
}

namespace {
Graphics::ProgramObjectRef subpixel_shader = NULL;
}


void PoissonTimingModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
	Graphics::gl_errors("PoissonTiming::draw");

}

void PoissonTimingModule::draw_stroke() {
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cons_fb);
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glPopAttrib();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void PoissonTimingModule::update(float elapsed_time) {
	if (!stroke.empty()) {
		draw_stroke();
		Vector2f end = stroke.back();
		stroke.clear();
		stroke.push_back(end);
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
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temp_fb);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glUseProgramObjectARB(jacobi_shader->handle);
			glUniform1fARB(glGetUniformLocationARB(jacobi_shader->handle, "scale"), jacobi_scale);
			jacobi_scale *= 0.9f;

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

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glPopAttrib();

			glActiveTextureARB(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glActiveTextureARB(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			swap(temp_fb, soln_fb);
			swap(temp_tex, soln_tex);

			iter_count() = iter_count() + 1;
		}

		glEnable(GL_BLEND);
	}
}


void PoissonTimingModule::draw_distance() {

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

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, soln_fb);

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

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	unsigned int step = 1;
	while (step > 0 && step < Size) {
		step *= 2;
	}
	assert(step != 0);

	unsigned int extra_step = 0;

	glUseProgramObjectARB(jfa_iter_shader->handle);
	while (step > 0 || extra_step > 0) { //jfa_iter part:

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temp_fb);

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

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temp_fb);

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

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		swap(temp_fb, soln_fb);
		swap(temp_tex, soln_tex);

		glUseProgramObjectARB(0);
	}



	glPopAttrib();

}

void PoissonTimingModule::do_gaussian() {
	vector< TexInfo > pyramid;
	vector< TexInfo > temp;

	pyramid.push_back(TexInfo());
	pyramid.back().size = make_vector(Size, Size);
	while (pyramid.back().size != make_vector(1U, 1U)) {
		Vector2ui new_size = pyramid.back().size;
		new_size.x = (new_size.x - 1) / 2 + 1;
		new_size.y = (new_size.y - 1) / 2 + 1;
		pyramid.push_back(TexInfo());
		pyramid.back().size = new_size;
	}
	temp = pyramid;
	temp.pop_back(); //don't need coarsest level.
	pyramid[0].tex = cons_tex;
	temp[0].tex = soln_tex;
	for (unsigned int i = 1; i < pyramid.size(); ++i) {
		pyramid[i].alloc(pyramid[i].size);
	}
	for (unsigned int i = 1; i < temp.size(); ++i) {
		temp[i].alloc(temp[i].size);
	}

	glPushAttrib(GL_VIEWPORT_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	static Graphics::ProgramObjectRef downsample_shader = NULL;
	static Graphics::ProgramObjectRef upsample_shader = NULL;

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

	glDisable(GL_BLEND);


	GLuint fb = 0;
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);

	Graphics::gl_errors("beforehand");

	//Downsample:
	glUseProgramObjectARB(downsample_shader->handle);
	for (unsigned int i = 1; i < pyramid.size(); ++i) {
		glViewport(0,0,pyramid[i].size.x, pyramid[i].size.y);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, pyramid[i].tex, 0);
		{ //check:
			GLenum ret = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER );
			if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
				std::cerr << "WARNING: FRAMEBUFFER not complete!" << std::endl;
			}
		}

		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, pyramid[i-1].tex);

		glBegin(GL_QUADS);
		const float Base = -0.5f;
		glTexCoord2f(Base, Base);
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(Base + pyramid[i].size.x * 2.0, Base);
		glVertex2f( 1.0f, -1.0f);
		glTexCoord2f(Base + pyramid[i].size.x * 2.0, Base + pyramid[i].size.y * 2.0);
		glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(Base, Base + pyramid[i].size.y * 2.0);
		glVertex2f(-1.0f,  1.0f);
		glEnd();

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);

		Graphics::gl_errors("downsample");
	}
	glUseProgramObjectARB(0);

	//Upsample:
	glUseProgramObjectARB(upsample_shader->handle);
	for (unsigned int i = temp.size() - 1; i < temp.size(); --i) {
		glViewport(0,0,temp[i].size.x, temp[i].size.y);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, temp[i].tex, 0);

		glActiveTexture(GL_TEXTURE1);
		if (i + 1 >= temp.size()) {
			assert(i + 2 == pyramid.size());
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, pyramid[i+1].tex);
		} else {
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, temp[i+1].tex);
		}
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, pyramid[i].tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(temp[i].size.x, 0);
		glVertex2f( 1.0f, -1.0f);
		glTexCoord2f(temp[i].size.x, temp[i].size.y);
		glVertex2f( 1.0f,  1.0f);
		glTexCoord2f(0, temp[i].size.y);
		glVertex2f(-1.0f,  1.0f);
		glEnd();

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

		Graphics::gl_errors("upsample");

	}
	glUseProgramObjectARB(0);

	glEnable(GL_BLEND);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fb);

	glPopAttrib();

	pyramid[0].tex = 0;
	temp[0].tex = 0;
	for (unsigned int i = 1; i < pyramid.size(); ++i) {
		pyramid[i].dealloc();
	}
	for (unsigned int i = 1; i < temp.size(); ++i) {
		temp[i].dealloc();
	}
}

bool PoissonTimingModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			stroke.push_back(local_mouse);
			show = SHOW_CONS;
			return true;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			if (!stroke.empty()) {
				stroke.push_back(local_mouse);
				draw_stroke();
				stroke.clear();
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cons_fb);
				glClear(GL_DEPTH_BUFFER_BIT);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

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
				color_b = color_a;
			}
			return true;
		}
		if (event.key.keysym.sym == SDLK_s) {
			show = SHOW_SOLN;
			return true;
		}
		if (event.key.keysym.sym == SDLK_c) {
			show = SHOW_CONS;
			return true;
		}
		if (event.key.keysym.sym == SDLK_d) {
			show = SHOW_SOLN;
			run = step = false;
			draw_distance();
			jacobi_scale = 1.0f;
			return true;
		}
		if (event.key.keysym.sym == SDLK_g) {
			run = step = false;
			show = SHOW_SOLN;
			Uint32 before_time = SDL_GetTicks();
			const unsigned int Iters = 10;
			for (unsigned int iter = 0; iter < Iters; ++iter) {
				do_gaussian();
				glFlush();
				glFinish();
			}
			Uint32 after_time = SDL_GetTicks();
			std::cout << "Took: " << after_time - before_time << " milliseconds for " << Iters << " iterations of gaussian. That's " << (after_time - before_time) / float(Iters) << " per." << std::endl;

			return true;
		}
		if (event.key.keysym.sym == SDLK_t) {
			show = SHOW_SOLN;
			run = step = false;
			Uint32 before_time = SDL_GetTicks();
			const unsigned int Iters = 10;
			for (unsigned int iter = 0; iter < Iters; ++iter) {
				draw_distance();
				jacobi_scale = 1.0f;
				for (unsigned int i = 0; i < 7; ++i) {
					step = true;
					update(0.0f);
				}
				glFlush();
				glFinish();
			}
			Uint32 after_time = SDL_GetTicks();
			std::cout << "Took: " << after_time - before_time << " milliseconds for " << Iters << " iterations of poisson. That's " << (after_time - before_time) / float(Iters) << " per." << std::endl;
			return true;
		}
		if (event.key.keysym.sym == SDLK_SPACE) {
			step = true;
			run = false;
			show = SHOW_SOLN;
			return true;
		}
		if (event.key.keysym.sym == SDLK_BACKSPACE) {
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cons_fb);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, soln_fb);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			iter_count() = 0;
			return true;
		}
	}
	return false;
}
