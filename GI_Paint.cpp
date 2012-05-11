#include "GI_Paint.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/load_save_png.hpp>

#include <vector>
#include <set>
#include <fstream>
#include <sstream>

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

using std::istringstream;
using std::swap;

StrokeBrush::StrokeBrush(float _softness, float _radius) : rate(5.0f), flow(1.0f), radius(_radius), softness(_softness), tex(0), tex_softness(-1.0f) {

}

const unsigned int BrushSize = 512;

GLuint StrokeBrush::get_tex() {
	if (tex == 0 || tex_softness != softness) {
		if (tex == 0) {
			glGenTextures(1, &tex);
		}
		tex_softness = softness;
		
		vector< float > brush(BrushSize * BrushSize);

		for (unsigned int y = 0; y < BrushSize; ++y) {
			for (unsigned int x = 0; x < BrushSize; ++x) {
				Vector2f p;
				p.x = 2.0f * ((x + 0.5f) / BrushSize - 0.5f);
				p.y = 2.0f * ((y + 0.5f) / BrushSize - 0.5f);

				float rad = length(p);

				float &out = brush[y * BrushSize + x];
				if (rad >= 1.0f) {
					out = 0.0f;
				} else if (rad >= 1.0f - softness) {
					out = (1.0f - rad) / softness;
				} else {
					out = 1.0f;
				}
			}
		}

		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float col[4] = {1.0f, 1.0f, 1.0f, 0.0f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, col);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BrushSize, BrushSize, 0, GL_ALPHA, GL_FLOAT, &brush[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		Graphics::gl_errors("build brush texture");
	}
	return tex;
}


namespace {
	Module *create_module(const std::string &params_in) {
		istringstream params(params_in);
		Vector2ui size = make_vector(0U, 0U);
		if (params >> size.x >> size.y) {
		} else {
			size = make_vector(128U, 128U);
			LOG_WARNING("Couldn't parse paint size, using default.");
		}
		return new Paint(size);
	}
	class Fred {
	public:
		Fred() {
			register_module("gi_paint", create_module, "width height");
		}
	} fred;
}

using std::vector;

REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_EXT_blend_func_separate )


namespace {
void alloc_tex(Vector2ui size, GLuint *tex, GLenum type = GL_RGBA) {
	assert(tex);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	vector< uint32_t > blank(size.x * size.y, 0);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, type, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &blank[0]);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc_tex");
}

void alloc_fb(GLuint tex, GLuint *fb) {
	assert(fb);
	glGenFramebuffers(1, fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *fb);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
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

Graphics::ProgramObjectRef paint_shader = NULL;
Graphics::ProgramObjectRef erase_shader = NULL;
}


Paint::Paint(Vector2ui const &_size) : cursor_fade(0.0f), have_cursor(false), stroke_tex(0), stroke_fb(0), temp_tex(0), temp_fb(0), fb(0), cur_mouse(make_vector(0.0f, 0.0f)), along(0.0f), erase(false), brush_spin_acc(0.0f) {

	alloc_tex(_size, &stroke_tex);
	alloc_fb(stroke_tex, &stroke_fb);

	alloc_tex(_size, &temp_tex);
	alloc_fb(temp_tex, &temp_fb);


	tex().alloc(_size);
	alloc_fb(tex().tex, &fb);

	in_ports.push_back(&color);
	color.name = "color";
	color.position = make_vector(-0.5f, 0.0f);

	out_ports.push_back(&tex);
	tex.name = "tex";
	tex.position = make_vector(0.5f, 0.0f);

	if (paint_shader.ref == NULL) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object_from_text(
			"#extension GL_ARB_texture_rectangle : enable \n"
			"uniform sampler2DRect stroke; \n"
			"uniform sampler2DRect canvas; \n"
			"uniform vec3 color; \n"
			"void main() { \n"
			"	vec2 at = gl_TexCoord[0].xy; \n"
			"	vec4 can = texture2DRect(canvas, at); \n"
			"	float amt = texture2DRect(stroke, at).r; \n"
			"	vec3 num = can.xyz * (1.0 - amt) * can.w + color * amt; \n"
			"	float den = (1.0 - amt) * can.w + amt; \n"
			"	if (den != 0.0) { \n"
			"		num /= den; \n"
			"	} \n"
			"	gl_FragColor = vec4(num, mix(can.w, 1.0, amt)); \n"
			"} \n",
			GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		paint_shader = get_program_object(frag);
		assert(paint_shader.ref);
	}
	if (erase_shader.ref == NULL) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object_from_text(
			"#extension GL_ARB_texture_rectangle : enable \n"
			"uniform sampler2DRect stroke; \n"
			"uniform sampler2DRect canvas; \n"
			"void main() { \n"
			"	vec2 at = gl_TexCoord[0].xy; \n"
			"	vec4 can = texture2DRect(canvas, at); \n"
			"	float amt = texture2DRect(stroke, at).r; \n"
			"	gl_FragColor = vec4(can.xyz, mix(can.w, 0.0, amt)); \n"
			"} \n",
			GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		erase_shader = get_program_object(frag);
		assert(erase_shader.ref);
	}


}

Paint::~Paint() {
	tex().dealloc();
}

bool Paint::handle_event(SDL_Event const &e, Vector2f ext_mouse) {
	if (e.type == SDL_MOUSEMOTION) {
		cur_mouse = (ext_mouse + size() * 0.5f) * tex().size.y;
		if (!acc.empty()) {
			acc.push_back(make_vector(cur_mouse, brush.radius, brush.flow));
			return true;
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELUP) {
		brush.radius *= 1.1f;
		return true;
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELDOWN) {
		brush.radius *= 1 / 1.1f;
		if (brush.radius < 0.5f) brush.radius = 0.5f;
		return true;
	}

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		if (acc.empty()) {
			erase = (SDL_GetModState() & KMOD_SHIFT);

			//Clear stroke texture:
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, stroke_fb);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			//Copy current drawing into temp:
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, temp_fb);

			glPushAttrib(GL_VIEWPORT_BIT);
			glViewport(0,0,tex().size.x,tex().size.y);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex().tex);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);

			glDisable(GL_BLEND);

			glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glTexCoord2f(0, 0);
			glVertex2f(-1.0f,-1.0f);
			glTexCoord2f(tex().size.x, 0);
			glVertex2f( 1.0f,-1.0f);
			glTexCoord2f(tex().size.x, tex().size.y);
			glVertex2f( 1.0f, 1.0f);
			glTexCoord2f(0, tex().size.y);
			glVertex2f(-1.0f, 1.0f);
			glEnd();

			glEnable(GL_BLEND);

			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);


			glPopAttrib();
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			Graphics::gl_errors("cloning fb");

		}
		acc.push_back(make_vector(cur_mouse, brush.radius, brush.flow)); 
		along = -1.0f;
		render_stroke();
		return true;
	}
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		if (!acc.empty()) {
			render_stroke();
			along = -1.0f;
			render_stroke();
			acc.clear();
			//clear stroke fb as well:
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, stroke_fb);
			glClearColor(1.0, 1.0, 1.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			return true;
		}
	}

	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
		if (fb) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}
		return true;
	}

	return false;
}

//Taken from soft_paint, because, um, that's just how I roll.
void Paint::render_stroke() {
	if (acc.empty()) return;

	vector< Vector4f > splats;
	if (along < 0.0f) {
		//special flag for "draw the first point"
		splats.push_back(acc[0]);
		along = 0.0f;
	}
	for (unsigned int p = 0; p + 1 < acc.size(); ++p) {
		Vector4f a = acc[p];
		Vector4f b = acc[p+1];
		//draw splat whenever along > radius / rate
		float a_thresh = a.z / brush.rate;
		if (a_thresh < 0.5f) a_thresh = 0.5f; //at least a half-pixel between splats.
		float b_thresh = b.z / brush.rate;
		if (b_thresh < 0.5f) b_thresh = 0.5f;
		//So thresh is changing from a_thresh to b_thresh.
		//while along is increasing from (along + 0) to (along + d).
		float d = length(b.xy - a.xy);
		
		float dslack = (b_thresh - a_thresh) - d;

		float t = 0.0f;
		while (t < 1.0f) {
			float slack = ((b_thresh - a_thresh) * t + a_thresh) - along;
			if (slack < 0) {
				//don't move forward, just emit:
				along = 0.0f;
				splats.push_back((b - a) * t + a);
			} else if (dslack * (1.0f - t) < -slack) {
				float delta =-slack / dslack;
				t += delta;
				along = 0.0f;
				splats.push_back((b - a) * t + a);
			} else {
				along += (1.0f - t) * d;
				break;
			}
		}
	}

	//Clear out points in the stroke accumulator we've used:
	// (last point is saved in case we need to continue)
	acc.erase(acc.begin(), acc.end()-1);

	if (splats.empty()) return;

	//Calculate verts for stroke:
	vector< Vector2f > quads(splats.size()*4);
	vector< Vector2f > texcoords(splats.size()*4);
	vector< Vector4f > colors(splats.size()*4);

	Vector4f col = make_vector(1.0f, 1.0f, 1.0f, 1.0f);
	//Remember where this stroke lands so we can draw in those tiles:
	for (vector< Vector4f >::iterator s = splats.begin(); s != splats.end(); ++s) {
		unsigned int ind = s - splats.begin();
		quads[4*ind+0] = make_vector(s->x - s->z, s->y - s->z);
		quads[4*ind+1] = make_vector(s->x + s->z, s->y - s->z);
		quads[4*ind+2] = make_vector(s->x + s->z, s->y + s->z);
		quads[4*ind+3] = make_vector(s->x - s->z, s->y + s->z);

		texcoords[4*ind+0] = make_vector(0.0f, 0.0f);
		texcoords[4*ind+1] = make_vector(1.0f, 0.0f);
		texcoords[4*ind+2] = make_vector(1.0f, 1.0f);
		texcoords[4*ind+3] = make_vector(0.0f, 1.0f);

		colors[4*ind+0] = make_vector(col.xyz, col.w * s->w);
		colors[4*ind+1] = make_vector(col.xyz, col.w * s->w);
		colors[4*ind+2] = make_vector(col.xyz, col.w * s->w);
		colors[4*ind+3] = make_vector(col.xyz, col.w * s->w);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, stroke_fb);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,tex().size.x,tex().size.y);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-1.0f, -1.0f, 0.0f);
	glScalef(2.0f / tex().size.x, 2.0f / tex().size.y, 1.0f);



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, brush.get_tex());
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &quads[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2f), &texcoords[0]);
	glColorPointer(4, GL_FLOAT, sizeof(Vector4f), &colors[0]);

	glDrawArrays(GL_QUADS, 0, quads.size());

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glPopAttrib();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//composite current stroke atop painting:

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,tex().size.x,tex().size.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (erase) {
		glUseProgramObjectARB(erase_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(erase_shader->handle, "canvas"), 0);
		glUniform1iARB(glGetUniformLocationARB(erase_shader->handle, "stroke"), 1);
	} else {
		glUseProgramObjectARB(paint_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(paint_shader->handle, "canvas"), 0);
		glUniform1iARB(glGetUniformLocationARB(paint_shader->handle, "stroke"), 1);
		glUniform3fARB(glGetUniformLocationARB(paint_shader->handle, "color"), color().x, color().y, color().z);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, stroke_tex);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, temp_tex);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glTexCoord2f(0, 0);
	glVertex2f(-1.0f,-1.0f);
	glTexCoord2f(tex().size.x, 0);
	glVertex2f( 1.0f,-1.0f);
	glTexCoord2f(tex().size.x, tex().size.y);
	glVertex2f( 1.0f, 1.0f);
	glTexCoord2f(0, tex().size.y);
	glVertex2f(-1.0f, 1.0f);
	glEnd();

	glEnable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);


	glUseProgramObjectARB(0);

	glPopAttrib();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	Graphics::gl_errors("stroke");
}


void Paint::update(float elapsed) {
	if (have_cursor) {
		cursor_fade += 10.0f * elapsed;
		if (cursor_fade > 1.0f) cursor_fade = 1.0f;
	} else {
		cursor_fade -= 4.0f * elapsed;
		if (cursor_fade < 0.0f) cursor_fade = 0.0f;
	}
	have_cursor = false;

	if (!acc.empty()) {
		brush_spin_acc = fmodf(brush_spin_acc + elapsed, 1.0f);
	}

	//Strokes:
	if (!acc.empty()) {
		//Paint some stacking weights:
		render_stroke();
	}
}

Vector2f Paint::size() {
	if (tex().size.y == 0) return make_vector(0.5f, 0.5f);
	else return make_vector(float(tex().size.x) / tex().size.y, 1.0f);
}

float Paint::cursor_alpha() {
	have_cursor = true;
	return (1.0f - cursor_fade);
}

void Paint::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f * size().x, -0.5f * size().y, 0.0f);
	if (tex().size.y != 0) {
		glScalef(1.0f / tex().size.y, 1.0f / tex().size.y, 1.0f);
	}


	{ //draw layer bounds
		glColor3f(0.5f, 0.1f, 0.1f);
		glBegin(GL_LINE_LOOP);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(tex().size.x, 0.0f);
		glVertex2f(tex().size.x, tex().size.y);
		glVertex2f(0, tex().size.y);
		glEnd();
		glLineWidth(1.0f);
	}

	assert(tex().tex);
	{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex().tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f( tex().size.x, 0.0f ); glVertex2f(tex().size.x,   0.0f);
		glTexCoord2f( tex().size.x, tex().size.y ); glVertex2f(tex().size.x, tex().size.y);
		glTexCoord2f( 0.0f, tex().size.y ); glVertex2f(  0.0f, tex().size.y);
		glEnd();

		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}


	{
		//brush
		Vector2f at = cur_mouse;
		unsigned int segs = (brush.radius * 4.0f * M_PI) / 7.0f + 5.0f;
		if (segs % 2) segs += 1;
		glLineWidth(4.5f);
		glColor4f(0.0f, 0.0f, 0.0f, 0.2f * cursor_fade);
		glBegin(GL_LINE_LOOP);
		for (unsigned int a = 0; a < segs; ++a) {
			float ang = float(a + brush_spin_acc * 10.0f) / float(segs) * 2.0f * float(M_PI);
			Vector2f b = make_vector(cosf(ang), sinf(ang));
			glVertex(b * 0.5f * brush.radius * 2.0f + at);
		}
		glEnd();
		glLineWidth(2.7f);
		glBegin(GL_LINES);
		for (unsigned int a = 0; a < segs; ++a) {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f * cursor_fade);
			float ang = float(a + brush_spin_acc * 10.0f) / float(segs) * 2.0f * float(M_PI);
			Vector2f b = make_vector(cosf(ang), sinf(ang));
			glVertex(b * 0.5f * brush.radius * 2.0f + at);
		}
		glEnd();

		glLineWidth(1.0f);
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


	Graphics::gl_errors("draw");
}
