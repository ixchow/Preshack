#include "GP_SimplePaintModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>

#include <Vector/VectorGL.hpp>

#include <iostream>
#include <sstream>

#include <vector>

#undef ERROR
#define ERROR( X ) std::cerr << X << std::endl
#define WARNING( X ) std::cout << X << std::endl
#define INFO( X ) std::cout << X << std::endl

using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::swap;
using std::ostringstream;
using std::istringstream;



namespace {
	Module *create_module(const std::string &params_in) {
		string params = params_in;
		SimplePaintModule *m = new SimplePaintModule(params);
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("simplepaint", create_module, "rezspec");
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

#include "gp_hsv_helpers.hpp"

SimplePaintModule::SimplePaintModule(string const &filename) {

	drew_recently = true;
	brush_fade = 0.0f;

	painting = false;
	brush_size = 4;

	color = make_vector(1.0f, 1.0f, 1.0f);
	stroke.Step = 1.0f;
	stroke.smoothing_kernel.clear();
	{ //a tad smoother.
		float sum = 0.0f;
		for (int x = -4; x <= 4; ++x) {
			float v = x / float(2.0f);
			stroke.smoothing_kernel.push_back(expf(-v*v));
			sum += stroke.smoothing_kernel.back();
		}
		INFO("Kernel sum is " << sum);
		for (unsigned int i = 0; i < stroke.smoothing_kernel.size(); ++i) {
			stroke.smoothing_kernel[i] /= sum;
		}
	}
	color_selecting = false;

	//--------------------------------------------------
	//Load an image, possibly:
	{
		istringstream dim(filename);
		char c;
		if (dim >> ImageWidth >> c >> ImageHeight && c == 'x') {
			INFO("Interpreting filename as image dimensions.");
			if (ImageWidth * ImageHeight > 100000000) {
				INFO("Image too big, I'm not going to allocate that for you.");
				ImageWidth = ImageHeight = 1024;
			}
		} else {
			ImageWidth = ImageHeight = 256;
		}
	}

	vector< Vector3f > f(ImageWidth * ImageHeight, make_vector< float, 3 >(0.5f));

	//-------------------------------------------------
	//Set up textures:
	
	alloc_tex(ImageWidth, ImageHeight, f_tex, f);
	alloc_fb(f_tex, f_fb);

	//--------------------------------------------------

	//set up proper (lack of) color clamping:
	if (have_ARB_color_buffer_float()) {
		glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
		Graphics::gl_errors("Color clampping");
	} else {
		WARNING("Unable to set colors unclamped [no ARB_color_buffer_float] -- you may have problems.");
	}

	mouse_pos = make_vector(0.0f, 0.0f);

	//--------------------------------------------------
	{
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/gp_scale_bias_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		scale_bias_subpixel_shader = Graphics::get_program_object(frag);
		assert(scale_bias_subpixel_shader.ref);
		glUseProgramObjectARB(scale_bias_subpixel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}

	//-------------------------------------------------

	Graphics::gl_errors("init");

}

SimplePaintModule::~SimplePaintModule() {
}

Vector2f SimplePaintModule::size() {
	return make_vector(2.0f, 2.0f / ImageWidth * ImageHeight);
}

void SimplePaintModule::update(float elapsed_time) {
	if (brush_fade > 0.0f) {
		brush_fade -= elapsed_time;
		if (brush_fade < 0.0f) brush_fade = 0.0f;
	}
	if (!drew_recently) return;
	drew_recently = false;
	glDisable(GL_BLEND);

	if (stroke.new_points || stroke.dirty) {
		render_stroke();
	}

	glEnable(GL_BLEND);
	Graphics::gl_errors("update");
}

bool SimplePaintModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	drew_recently = true;
	//so we can handle drawing:
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_MIDDLE) {
		return false;
	}

	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELUP) {
		brush_size -= 1;
		if (brush_size < 0.5f) brush_size = 0.5f;
		brush_fade = 1.0f;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_WHEELDOWN) {
		brush_size += 1;
		brush_fade = 1.0f;
		return true;
	}

	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT ) {
		color_selecting = true;
		brush_fade = 1.0f;
	}
	//Stop edge drawing:
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
		color_selecting = false;
		brush_fade = 1.0f;
	}
	//Stop Painting:
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		painting = false;
		brush_fade = 1.0f;
	}
	//Start Painting:
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		Vector2f draw_pos = mouse_pos;
		draw_pos.y = ImageHeight - draw_pos.y;
		if (SDL_GetModState() & KMOD_SHIFT) {
			//SHIFT -> draw line.
			stroke.submit_point(draw_pos);
		} else {
			//start new stroke:
			painting = true;
			stroke.clear();
			stroke.submit_point(draw_pos);
		}
		brush_fade = 1.0f;
	}
	//Actually paint:
	if (event.type == SDL_MOUSEMOTION) {
		//track motion 'direction':
		Vector2f old_pos = mouse_pos;
		mouse_pos.x = (local_mouse.x / size().x + 0.5f) * ImageWidth;
		mouse_pos.y = (local_mouse.y / size().y + 0.5f) * ImageHeight;

		if (color_selecting) {
			Vector3f hsv = rgb_to_hsv(color);
			bool do_value = (SDL_GetModState() & KMOD_SHIFT) != 0;
			Vector2f pos;
			if (do_value) {
				pos.x = (hsv.v - 0.5f) * 2.0f * ColorSelRad;
				pos.y = 0.0f;
			} else {
				pos.x = cosf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
				pos.y = sinf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
			}
			pos += mouse_pos - old_pos;
			if (do_value) {
				hsv.v = pos.x / (2.0f * ColorSelRad) + 0.5f;
				if (hsv.v < 0.0f) hsv.v = 0.0f;
				if (hsv.v > 1.0f) hsv.v = 1.0f;
			} else {
				hsv.h = atan2(pos.y, pos.x) / (2 * M_PI);
				if (hsv.h < 0.0f) hsv.h += 1.0f;
				hsv.s = length(pos) / ColorSelRad;
				if (hsv.s < 0.0f) hsv.s = 0.0f;
				if (hsv.s > 1.0f) hsv.s = 1.0f;
			}
			color = hsv_to_rgb(hsv);
			stroke.dirty = true;
		} else if (painting) {
			Vector2f draw_pos = mouse_pos;
			Vector2f draw_old_pos = old_pos;
			draw_pos.y = ImageHeight - draw_pos.y;
			draw_old_pos.y = ImageHeight - draw_old_pos.y;
			stroke.submit_point(draw_pos);
		}
		brush_fade = 1.0f;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
		brush_fade = 1.0f;
		return true;
	}
	return false;
}

float SimplePaintModule::cursor_alpha() {
	return 0.0f;
}

void SimplePaintModule::draw(Box2f viewport, Box2f screen_viewport, float, unsigned int recurse) {
	drew_recently = true;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);

	//background:
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();


	glDisable(GL_BLEND);

	//Move into a 'ImageWidthxImageHeight' screen:
	glPushMatrix();
	glTranslatef(-0.5f * size().x,-0.5f * size().y, 0.0f);
	glScalef(size().x / ImageWidth, size().y / ImageHeight, 1.0f);

	//and on with the show as per normal:

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	set_nearest(); //because we'll use subsample in shader.

	glUseProgramObjectARB(scale_bias_subpixel_shader->handle);
	glUniform1fARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "scale"), 1.0f);
	glUniform1fARB(glGetUniformLocationARB(scale_bias_subpixel_shader->handle, "bias"), 0.0f);
	float px_size = 2.0f / Graphics::screen_y * viewport.size().y / screen_viewport.size().y * ImageHeight / size().y;

	glColor3f(1.0f, 1.0f, 1.0f);

	glBegin(GL_QUADS);
	glTexCoord2f(0-0.5f*px_size,ImageHeight-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0+0.5f*px_size,ImageHeight+0.5f*px_size);
	glVertex2i(0,0);

	glTexCoord2f(ImageWidth-0.5f*px_size,ImageHeight-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,ImageWidth+0.5f*px_size,ImageHeight+0.5f*px_size);
	glVertex2i(ImageWidth,0);

	glTexCoord2f(ImageWidth-0.5f*px_size,0-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,ImageWidth+0.5f*px_size,0+0.5f*px_size);
	glVertex2i(ImageWidth,ImageHeight);

	glTexCoord2f(0-0.5f*px_size,0-0.5f*px_size);
	glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0+0.5f*px_size,0+0.5f*px_size);
	glVertex2i(0,ImageHeight);

	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	glEnable(GL_BLEND);

	float brush_alpha = 1.0f;
	if (brush_fade < 0.5f) brush_alpha = brush_fade / 0.5f;

	{ //cursor
		float w = 2.5f; //widths are multiples 'o this.

		glPushMatrix();
		glTranslatef(mouse_pos.x, mouse_pos.y, 0.0f);

		//brush outline:
		if (painting) {
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
				glVertex2f(brush_size * cosf(ang), brush_size * sinf(ang));
			}
			glEnd();
			glLineWidth(w * 0.9f);
			glColor3f(1.0f, 0.0f, 0.0f);
			glBegin(GL_LINE_LOOP);
			for (unsigned int a = 0; a < 32; ++a) {
				float ang = a / float(32) * M_PI * 2.0f;
				glVertex2f(brush_size * cosf(ang), brush_size * sinf(ang));
			}
			glEnd();
			glDisable(GL_LINE_STIPPLE);
			glPopMatrix();
		}

		//crosshair cursor:
		glLineWidth(2.5f);
		glBegin(GL_LINES);
		glColor4f(1.0f, 1.0f, 1.0f, brush_alpha);
		glVertex2f( 0.0f,-5.0f);
		glVertex2f( 0.0f, 5.0f);
		glVertex2f(-5.0f, 0.0f);
		glVertex2f( 5.0f, 0.0f);

		glColor4f(0.0f, 0.0f, 0.0f, brush_alpha);
		glVertex2f(brush_size, 0.0f);
		glVertex2f(brush_size+3, 0.0f);

		glVertex2f(-brush_size, 0.0f);
		glVertex2f(-brush_size-3, 0.0f);

		glVertex2f(0.0f, brush_size);
		glVertex2f(0.0f, brush_size+3);

		glVertex2f(0.0f,-brush_size);
		glVertex2f(0.0f,-brush_size-3);

		glColor4f(1.0f, 1.0f, 1.0f, brush_alpha);
		glVertex2f( brush_size,-3);
		glVertex2f( brush_size,+3);

		glVertex2f(-brush_size,-3);
		glVertex2f(-brush_size,+3);

		glVertex2f(-3, brush_size);
		glVertex2f( 3, brush_size);

		glVertex2f(-3,-brush_size);
		glVertex2f( 3,-brush_size);
		glEnd();

		glPopMatrix();
		glLineWidth(1.0f);
	}

	//Border (so paint selector can overlap it):
	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(ImageWidth, 0.0f);
	glVertex2f(ImageWidth,  ImageHeight);
	glVertex2f(0.0f,  ImageHeight);
	glEnd();

	if (color_selecting) {
		Vector3f hsv = rgb_to_hsv(color);
		Vector2f pos;
		bool do_value = (SDL_GetModState() & KMOD_SHIFT) != 0;
		if (do_value) {
			pos.x = (hsv.v - 0.5f) * 2.0f * ColorSelRad;
			pos.y = 0.0f;
		} else {
			pos.x = cosf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
			pos.y = sinf(hsv.h * 2.0f * float(M_PI)) * hsv.s * ColorSelRad;
		}
		glPushMatrix();
		glTranslatef(mouse_pos.x - pos.x, mouse_pos.y - pos.y, 0.0f);
		glScalef(ColorSelRad, ColorSelRad, 0.0f);
		float ofs = 0.02f;
		float dia = 0.04f;
		if (!do_value) {
			glPushMatrix();
			glTranslatef(ofs,-ofs, 0.0f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(0.0f, 0.0f);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glVertex2f(cosf(ang), sinf(ang));
			}
			glEnd();
			glBegin(GL_QUAD_STRIP);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
				glVertex2f(cosf(ang), sinf(ang));
				glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
				glVertex2f((1.0f + dia) * cosf(ang), (1.0f + dia) * sinf(ang));
			}
			glEnd();

			glPopMatrix();

			glBegin(GL_TRIANGLE_FAN);
			glColor(hsv_to_rgb(make_vector(hsv.h, 0.0f, 1.0f)));
			glVertex2f(0.0f, 0.0f);
			for (unsigned int a = 0; a <= 30; ++a) {
				float ang = a / 30.0f * float(M_PI) * 2.0f;
				glColor(hsv_to_rgb(make_vector(ang / (float(M_PI) * 2.0f), 1.0f, 1.0f)));
				glVertex2f(cosf(ang), sinf(ang));
			}
			glEnd();
		}
		if (do_value) {
			glPushMatrix();
			glTranslatef(ofs,-ofs, 0.0f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glBegin(GL_QUADS);
			glVertex2f(-1.0f, -0.1f);
			glVertex2f(-1.0f,  0.1f);
			glVertex2f( 1.0f,  0.1f);
			glVertex2f( 1.0f, -0.1f);
			glEnd();
			glBegin(GL_QUAD_STRIP);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia, -0.1f-dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f,  0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia,  0.1f+dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f( 1.0f,  0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f( 1.0f+dia,  0.1f+dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f( 1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f( 1.0f+dia, -0.1f-dia);
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertex2f(-1.0f, -0.1f);
			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
			glVertex2f(-1.0f-dia, -0.1f-dia);
			glEnd();
			glPopMatrix();
			glBegin(GL_QUADS);
			glColor(hsv_to_rgb(make_vector(hsv.h, hsv.s, 0.0f)));
			glVertex2f(-1.0f, -0.1f);
			glVertex2f(-1.0f,  0.1f);
			glColor(hsv_to_rgb(make_vector(hsv.h, hsv.s, 1.0f)));
			glVertex2f( 1.0f,  0.1f);
			glVertex2f( 1.0f, -0.1f);
			glEnd();
		}
		glScalef(1.0f / ColorSelRad, 1.0f / ColorSelRad, 0.0f);
		glBegin(GL_LINES);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(pos.x - 3.0f, pos.y);
		glVertex2f(pos.x + 3.0f, pos.y);
		glVertex2f(pos.x, pos.y - 3.0f);
		glVertex2f(pos.x, pos.y + 3.0f);
		glEnd();
		glPopMatrix();
	}

	//escape this crazy world:
	glPopMatrix();
	glEnable(GL_BLEND);


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);

	Graphics::gl_errors("draw");
}


void SimplePaintModule::render_stroke() {

	if (stroke.points.size() < 2) return;

	bind_fb(f_fb, ImageWidth, ImageHeight);

	Vector2f prev = stroke.points[0];
	Vector2f cur = stroke.points[1];
	Vector2f prev_perp = perpendicular(normalize(cur - prev));

	glBegin(GL_QUADS);
	for (unsigned int i = 2; i < stroke.points.size(); ++i) {
		Vector2f next = stroke.points[i];
		Vector2f next_perp = perpendicular(normalize(next - cur));

		#define VERT(P,D) do { \
			glVertex((P)); \
			} while(0)

		//twixt-segment blob:
		glColor(color, 1.0f);
		VERT(cur+next_perp * brush_size, DMAX);
		VERT(cur+prev_perp * brush_size, DMAX);
		VERT(cur, DMIN);
		VERT(cur, DMIN);
		//
		VERT(cur-next_perp * brush_size, DMAX);
		VERT(cur-prev_perp * brush_size, DMAX);
		VERT(cur, DMIN);
		VERT(cur, DMIN);

		//segment:
		glColor(color, 1.0f);
		VERT(cur+next_perp * brush_size, DMAX);
		VERT(next+next_perp * brush_size, DMAX);
		VERT(next, DMIN);
		VERT(cur, DMIN);
		//
		VERT(cur-next_perp * brush_size, DMAX);
		VERT(next-next_perp * brush_size, DMAX);
		VERT(next, DMIN);
		VERT(cur, DMIN);

		#undef VERT
		#undef TEX

		prev = cur;
		cur = next;
		prev_perp = next_perp;
	}
	glEnd();

	unbind_fb();

	stroke.new_points = 0;
	stroke.dirty = false;
}
