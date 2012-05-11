#include "SP_Paint.hpp"

#include <Graphics/Font.hpp>
#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/load_save_png.hpp>
#include <Vector/VectorGL.hpp>

#include <algorithm>
#include <sstream>

using std::swap;

#define LOG_ERROR( X ) std::cout << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl


namespace {
	Module *create_module(const std::string &params_in) {
		string params = params_in;
		vector< string > files;
		params += ' ';
		while (params.size()) {
			if (isspace(params[0])) {
				params.erase(0,1);
				continue;
			}
			for (unsigned int c = 0; c < params.size(); ++c) {
				if (isspace(params[c])) {
					files.push_back(params.substr(0,c));
					params.erase(0,c);
					break;
				}
			}
		}
		if (files.size() != 2) {
			LOG_INFO("Wanted exactly two layers.");
			return NULL;
		}
		vector< uint32_t > data_a, data_b;
		Vector2ui size_a, size_b;
		if (!load_png(files[0], size_a.x, size_a.y, data_a) || !load_png(files[1], size_b.x, size_b.y, data_b) || size_a != size_b) {
			LOG_INFO("Error loading.");
			return NULL;
		}

		return new SP_Paint(size_a, &data_a[0], &data_b[0]);
	}
	class Fred {
	public:
		Fred() {
			register_module("softpaint", create_module, "layerA layerB");
		}
	} fred;
}

REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);
REQUIRE_GL_EXTENSION(GL_ARB_framebuffer_object);
REQUIRE_GL_EXTENSION(GL_ARB_depth_texture);
REQUIRE_GL_EXTENSION(GL_ARB_draw_buffers);
REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);
REQUIRE_GL_EXTENSION(GL_EXT_texture_array);
REQUIRE_GL_EXTENSION(GL_EXT_blend_func_separate);

void check_fb(std::string name) {
	GLenum ret = glCheckFramebufferStatus( GL_FRAMEBUFFER_EXT );
	if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
		LOG_ERROR(name << " framebuffer NOT COMPLETE!");
	}
}

SP_Paint::SP_Paint(Vector2ui size, uint32_t *a_data, uint32_t *b_data) : a_tex(0), b_tex(0), order_tex(0), order_fb(0), canvas_size(size) {

	draw_cursor = 0.0f;
	old_mouse = make_vector(0.0f, 0.0f);

	{ //a_tex:
		glGenTextures(1, &a_tex);
		glBindTexture(GL_TEXTURE_2D, a_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, a_data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	{ //b_tex:
		glGenTextures(1, &b_tex);
		glBindTexture(GL_TEXTURE_2D, b_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, b_data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	{ //order_tex:
		vector< uint32_t > data(canvas_size.x * canvas_size.y, 0);
		glGenTextures(1, &order_tex);
		glBindTexture(GL_TEXTURE_2D, order_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	{ //order_fb:
		glGenFramebuffers(1, &order_fb);
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, order_fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, order_tex, 0);
		check_fb("order_fb");
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	}


	stroke_rad = 20.0f;
	stroke_color = make_vector(1.0f, 0.2f, 0.6f);
	stroke_acc = 0.0f;
	stroke_brush = 0;
	stroke_rate = 0.3f;
	stroke_pressure = 0.2f;
	{
		const unsigned int Size = 256;
		vector< uint32_t > brush(Size * Size);
		for (unsigned int y = 0; y < Size; ++y) {
			for (unsigned int x = 0; x < Size; ++x) {
				float d = length(make_vector(x - Size * 0.5f, y - Size * 0.5f));
				float amt = 1.0f - d / (Size * 0.25f); //DEBUG 0.5f - (d - Size * 0.25f);
				if (amt < 0.0f) amt = 0.0f;
				if (amt > 1.0f) amt = 1.0f;
				int c = (int)(amt * 255.0f);
				if (c < 0) c = 0;
				if (c > 255) c = 255;
				brush[y * Size + x] = 0xffffff | (c << 24);
			}
		}
		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Size, Size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &brush[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
		Graphics::gl_errors("make brush");
		brushes.push_back(tex);
	}
	{
		const unsigned int Size = 256;
		vector< uint32_t > brush(Size * Size);
		for (unsigned int y = 0; y < Size; ++y) {
			for (unsigned int x = 0; x < Size; ++x) {
				float d = length(make_vector(x - Size * 0.5f, y - Size * 0.5f));
				float amt = 1.0f - (d - Size * 0.25f);
				if (amt < 0.0f) amt = 0.0f;
				if (amt > 1.0f) amt = 1.0f;
				int c = (int)(amt * 255.0f);
				if (c < 0) c = 0;
				if (c > 255) c = 255;
				brush[y * Size + x] = 0xffffff | (c << 24);
			}
		}
		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Size, Size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &brush[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
		Graphics::gl_errors("make brush");
		brushes.push_back(tex);
	}
}

SP_Paint::~SP_Paint() {
	glDeleteFramebuffers(1, &order_fb);
	glDeleteTextures(1, &a_tex);
	glDeleteTextures(1, &b_tex);
	glDeleteTextures(1, &order_tex);
	glDeleteTextures(brushes.size(), &(brushes[0]));
	brushes.clear();
}

Vector2f SP_Paint::size() {
	return make_vector(1.0f, float(canvas_size.y) / canvas_size.x);
}


void SP_Paint::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	/*{ //background:
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(-size().x*0.5f, -size().y*0.5f);
		glVertex2f( size().x*0.5f, -size().y*0.5f);
		glVertex2f( size().x*0.5f,  size().y*0.5f);
		glVertex2f(-size().x*0.5f,  size().y*0.5f);
		glEnd();
	}*/

	static Graphics::ProgramObjectRef combine_shader = NULL;
	if (!combine_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("sp_shaders/combine.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		combine_shader = Graphics::get_program_object(frag);
		assert(combine_shader.ref);
		glUseProgramObjectARB(combine_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(combine_shader->handle, "layerA"), 0);
		glUniform1iARB(glGetUniformLocationARB(combine_shader->handle, "layerB"), 1);
		glUniform1iARB(glGetUniformLocationARB(combine_shader->handle, "order"), 2);
		glUseProgramObjectARB(0);
	}


	{
		glEnable(GL_BLEND);
		//Draw the tile!
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, order_tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, b_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, a_tex);

		glUseProgramObjectARB(combine_shader->handle);
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, order_tex);

		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-size().x*0.5f,-size().y*0.5f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-size().x*0.5f, size().y*0.5f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f( size().x*0.5f, size().y*0.5f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f( size().x*0.5f,-size().y*0.5f);
		glEnd();

		//glDisable(GL_TEXTURE_2D);
		glUseProgramObjectARB(0);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	{ //Brush stuff:
		glPushMatrix();
		glTranslatef(old_mouse.x, old_mouse.y, 0.0f);
		glScalef(size().x / canvas_size.x, size().y / canvas_size.y, 1.0f);


		//brush outline:
		if (!stroke_queue.empty()) {
			const float w = 2.5f;
			glPushMatrix();
			static float amt = 0.0f;
			amt = fmodf(amt + 1.0f, 360.0f);
			glRotatef(amt, 0,0,1);
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1, 0x00ff);
			glLineWidth(w * 1.5f);
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			glBegin(GL_LINE_LOOP);
			for (unsigned int a = 0; a < 32; ++a) {
				float ang = a / float(32) * M_PI * 2.0f;
				glVertex2f(stroke_rad * cosf(ang), stroke_rad * sinf(ang));
			}
			glEnd();
			glLineWidth(w * 0.9f);
			glColor(stroke_color);
			glBegin(GL_LINE_LOOP);
			for (unsigned int a = 0; a < 32; ++a) {
				float ang = a / float(32) * M_PI * 2.0f;
				glVertex2f(stroke_rad * cosf(ang), stroke_rad * sinf(ang));
			}
			glEnd();
			glDisable(GL_LINE_STIPPLE);
			glPopMatrix();
		}

		//crosshair cursor:
		glLineWidth(2.5f);
		glBegin(GL_LINES);
		glColor4f(1.0f, 1.0f, 1.0f, draw_cursor);
		glVertex2f( 0.0f,-5.0f);
		glVertex2f( 0.0f, 5.0f);
		glVertex2f(-5.0f, 0.0f);
		glVertex2f( 5.0f, 0.0f);

		glColor4f(0.0f, 0.0f, 0.0f, draw_cursor);
		glVertex2f(stroke_rad, 0.0f);
		glVertex2f(stroke_rad+3, 0.0f);

		glVertex2f(-stroke_rad, 0.0f);
		glVertex2f(-stroke_rad-3, 0.0f);

		glVertex2f(0.0f, stroke_rad);
		glVertex2f(0.0f, stroke_rad+3);

		glVertex2f(0.0f,-stroke_rad);
		glVertex2f(0.0f,-stroke_rad-3);

		glColor4f(1.0f, 1.0f, 1.0f, draw_cursor);
		glVertex2f( stroke_rad,-3);
		glVertex2f( stroke_rad,+3);

		glVertex2f(-stroke_rad,-3);
		glVertex2f(-stroke_rad,+3);

		glVertex2f(-3, stroke_rad);
		glVertex2f( 3, stroke_rad);

		glVertex2f(-3,-stroke_rad);
		glVertex2f( 3,-stroke_rad);
		glEnd();

		glPopMatrix();
		glLineWidth(1.0f);
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	Graphics::gl_errors("draw");

}

void SP_Paint::update(float elapsed) {
	render_stroke();
	draw_cursor -= elapsed * 10.0f;
	if (draw_cursor < 0.0f) {
		draw_cursor = 0.0f;
	}
}

void SP_Paint::render_stroke() {
	if (stroke_queue.size() < 2) return;
	//brush as series of splatted 'brush' pictures:
	vector< Vector2f > quads;
	vector< Vector2f > quads_tex;
	for (unsigned int i = 0; i + 1 < stroke_queue.size(); ++i) {
		Vector3f a = stroke_queue[i];
		Vector3f b = stroke_queue[i+1];
		float len = length(a-b);
		if (len == 0.0f) continue;
		float at = 0.0f;
		while (1) {
			float brush_start = ((b.z - a.z) * (at / len) + a.z) * stroke_rad * stroke_rate;
			float brush_slope = (b.z - a.z) / float(len) * stroke_rad * stroke_rate;
			float limit = len;
			if (brush_start < 0.5f) {
				brush_start = 0.5f;
				brush_slope = 0.0f;
				//figure out limit:
				if (brush_slope > 0 && brush_slope * (len - at) + brush_start > 0.5f) {
					limit = ((0.5f - brush_start) / brush_slope) + at;
					assert(limit <= len + 0.001f);
				}
			}
			//when is stroke_acc + at == brush_start + brush_slope * at?
			//(stroke_acc - brush_start) == (brush_slope - 1.0) * at

			if ((brush_slope - 1.0f) != 0.0f) {
				float test = (stroke_acc - brush_start) / (brush_slope - 1.0f) + at;
				if (test < limit) {
					at = test;
					stroke_acc = 0.0f;
					float size = ((b.z - a.z) * (at / len) + a.z) * stroke_rad;
					Vector2f c = (b.xy - a.xy) * (at / len) + a.xy;
					quads.push_back(make_vector(-2.0f,-2.0f)*size+c);
					quads.push_back(make_vector(-2.0f, 2.0f)*size+c);
					quads.push_back(make_vector( 2.0f, 2.0f)*size+c);
					quads.push_back(make_vector( 2.0f,-2.0f)*size+c);
					quads_tex.push_back(make_vector(0.0f,0.0f));
					quads_tex.push_back(make_vector(0.0f,1.0f));
					quads_tex.push_back(make_vector(1.0f,1.0f));
					quads_tex.push_back(make_vector(1.0f,0.0f));
					continue;
				}
			}
			stroke_acc += (len - at);
			break;
		}

	}
	
	//empty out stroke queue:
	swap(stroke_queue.back(),stroke_queue[0]);
	stroke_queue.erase(stroke_queue.begin()+1, stroke_queue.end());

	//bind quads to vertex pointer:
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &quads[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2f), &quads_tex[0]);
	glColor(stroke_color, stroke_pressure);

	//and render that mofo to our stroke tiles:
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,canvas_size.x, canvas_size.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(-1.0f,-1.0f, 0.0f);
	glScalef(2.0f / canvas_size.x, 2.0f / canvas_size.y,-0.9f); //watch out for end of depth buffer!
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	assert(stroke_brush < brushes.size());
	glBindTexture(GL_TEXTURE_2D, brushes[stroke_brush]);
	{
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, order_fb);
		glEnable(GL_BLEND);
		glBlendFuncSeparateEXT(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			
		glDrawArrays(GL_QUADS, 0, quads.size());

		glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	}
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPopAttrib();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	Graphics::gl_errors("stroke");
}

Vector4f over(Vector4f a, Vector4f b) {
	if (a.a + b.a == 0.0f) {
		return make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	}
	return make_vector((a.xyz * a.w + b.xyz * (1.0f - a.w) * b.w) / (a.w + (1.0f - a.w) * b.w), a.w + (1.0f - a.w) * b.w);
}

bool SP_Paint::handle_event(SDL_Event const &e, Vector2f mouse) {
	if (e.type == SDL_MOUSEMOTION) {
		old_mouse = mouse;
		if (e.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			Vector2f world_after = mouse * canvas_size.x / size().x + make_vector< float >(canvas_size) * 0.5f;
			stroke_queue.push_back(make_vector(world_after, 1.0f));
			return true;
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		//start a stroke if on canvas:
		Vector2f world = mouse * canvas_size.x / size().x + make_vector< float >(canvas_size) * 0.5f;
		stroke_queue.push_back(make_vector(world, 1.0f));
		stroke_queue.push_back(make_vector(world, 1.0f));
		stroke_acc = 0.0f; //special value means "always splat now"
		return true;
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELDOWN) {
		stroke_rad -= 1.0f;
		if (stroke_rad < 1.0f) {
			stroke_rad = 1.0f;
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELUP) {
		stroke_rad += 1.0f;
		if (stroke_rad > 400.0f) {
			stroke_rad = 400.0f;
		}
	}
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		if (!stroke_queue.empty()) {
			render_stroke();
			stroke_queue.clear();
		}
		return true;
	}
	if (e.type == SDL_KEYDOWN) {
		if (e.key.keysym.sym == SDLK_1) {
			stroke_color = make_vector(1.0f, 0.0f, 0.0f);
			return true;
		}
		if (e.key.keysym.sym == SDLK_2) {
			stroke_color = make_vector(0.0f, 0.0f, 1.0f);
			return true;
		}
		if (e.key.keysym.sym == SDLK_RIGHT) {
			stroke_brush = (stroke_brush + 1) % brushes.size();
			return true;
		}
		if (e.key.keysym.sym == SDLK_BACKSPACE) {
			vector< uint32_t > data(canvas_size.x * canvas_size.y, 0);
			glBindTexture(GL_TEXTURE_2D, order_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
			return true;
		}
		if (e.key.keysym.sym == SDLK_s) {
			vector< Vector4f > a(canvas_size.x * canvas_size.y);
			vector< Vector4f > b(canvas_size.x * canvas_size.y);
			vector< Vector4f > order(canvas_size.x * canvas_size.y);
			glBindTexture(GL_TEXTURE_2D, a_tex);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &a[0]);
			glBindTexture(GL_TEXTURE_2D, b_tex);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &b[0]);
			glBindTexture(GL_TEXTURE_2D, order_tex);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &order[0]);
			glBindTexture(GL_TEXTURE_2D, 0);
			vector< uint32_t > out(a.size());
			for (unsigned int i = 0; i < a.size(); ++i) {
				Vector4f ab = over(a[i],b[i]);
				Vector4f ba = over(b[i],a[i]);
				Vector4f col = ab + order[i].x * (ba - ab);
				uint32_t res = 0;
				for (unsigned int c = 0; c < 4; ++c) {
					int amt = col[c] * 256;
					if (amt > 255) amt = 255;
					if (amt < 0) amt = 0;
					res |= (amt << (c * 8));
				}
				out[i] = res;
			}
			save_png("sp_dump.png", canvas_size.x, canvas_size.y, &out[0]);
		}
	}
	return false;
}

float SP_Paint::cursor_alpha() {
	draw_cursor = 1.0f;
	return 0.0f;
}
