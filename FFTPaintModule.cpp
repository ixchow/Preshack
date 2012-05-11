#include "FFTPaintModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>
#include <algorithm>

#include "GradientStuff.hpp"

REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_ARB_multitexture )

using std::istringstream;
using std::swap;

namespace {
	Module *create_module(const std::string &params) {
		init_gradient_stuff();
		FFTPaintModule *p = new FFTPaintModule();
		p->filename = params;
		if (params != "") {
			p->load();
		}
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("fft_paint", create_module);
		}
	} fred;
}

Vector2f FFTPaintModule::size() {
	return make_vector(2.0f, 2.0f);
}

void FFTPaintModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	if (show == SHOW_GXR || show == SHOW_GYR) {
		calc_residual_gradients();
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

	//---------------------------------------

	if (show == SHOW_F) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	} else if (show == SHOW_RHS) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rhs_tex);
	} else if (show == SHOW_GX) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
	} else if (show == SHOW_GXR) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_residual_tex);
	} else if (show == SHOW_GY) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
	} else if (show == SHOW_GYR) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_residual_tex);
	}
	glUseProgramObjectARB(YCbCr_shader->handle);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	set_view_factors();
	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, SimpleHeight); glVertex2f(-1.0f,-1.0f);
	glTexCoord2f(SimpleWidth, SimpleHeight); glVertex2f( 1.0f,-1.0f);
	glTexCoord2f(SimpleWidth, 0.0f); glVertex2f( 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
	glEnd();

	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	glEnable(GL_BLEND);

	glPushMatrix();
	glTranslatef(-1.0f, 1.0f, 0.0f);
	glScalef(2.0f / SimpleWidth,-2.0f / SimpleHeight, 1.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	glVertex2f(mouse_pos.x, mouse_pos.y - 5);
	glVertex2f(mouse_pos.x, mouse_pos.y + 5);
	glVertex2f(mouse_pos.x-5, mouse_pos.y);
	glVertex2f(mouse_pos.x+5, mouse_pos.y);

	glVertex2f(mouse_pos.x + brush_size, mouse_pos.y - 3);
	glVertex2f(mouse_pos.x + brush_size, mouse_pos.y + 3);

	glVertex2f(mouse_pos.x - brush_size, mouse_pos.y - 3);
	glVertex2f(mouse_pos.x - brush_size, mouse_pos.y + 3);

	glVertex2f(mouse_pos.x-3, mouse_pos.y + brush_size);
	glVertex2f(mouse_pos.x+3, mouse_pos.y + brush_size);

	glVertex2f(mouse_pos.x-3, mouse_pos.y - brush_size);
	glVertex2f(mouse_pos.x+3, mouse_pos.y - brush_size);

	if (clone) {
		glVertex2f(clone_point.x, clone_point.y - 5);
		glVertex2f(clone_point.x, clone_point.y + 5);
		glVertex2f(clone_point.x-5, clone_point.y);
		glVertex2f(clone_point.x+5, clone_point.y);

		if (painting) {
			glVertex2f(mouse_pos.x + clone_offset.x, mouse_pos.y + clone_offset.y - 5);
			glVertex2f(mouse_pos.x + clone_offset.x, mouse_pos.y + clone_offset.y + 5);
			glVertex2f(mouse_pos.x-5 + clone_offset.x, mouse_pos.y + clone_offset.y);
			glVertex2f(mouse_pos.x+5 + clone_offset.x, mouse_pos.y + clone_offset.y);
		}
	}

	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(mouse_pos.x, mouse_pos.y);
	glVertex2f(mouse_pos.x+mouse_dir.x * 10, mouse_pos.y+mouse_dir.y * 10);
	glEnd();

	glPopMatrix();

	//---------------------------------------
	string info = "";
	if (relax_mode) {
		info += "iter";
	}
	if (show == SHOW_GX) {
		info += ": gx";
	}
	if (show == SHOW_GY) {
		info += ": gy";
	}
	if (show == SHOW_RHS) {
		info += ": rhs";
	}
	if (show == SHOW_GXR) {
		info += ": gx residual";
	}
	if (show == SHOW_GYR) {
		info += ": gy residual";
	}
	if (freq_edit) {
		info += "(lim)";
	}
	{
		Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
		gentium->draw(info, make_vector(-1.0f, -1.0f), 0.1f);
	}

	//---------------------------------------

	glBegin(GL_LINE_LOOP);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("FFTPaint::draw");

}

void FFTPaintModule::update(float elapsed_time) {
	if (grad_dirty || relax_mode) {
		grad_dirty = false;

		//--------------------------------
		//Construct RHS:
		glDisable(GL_BLEND);

		bind_fb(rhs_fb, SimpleWidth, SimpleHeight);

		glActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
		set_clamp_to_black();
		glActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
		set_clamp_to_black();

		glUseProgramObjectARB(rhs_shader->handle);

		glBegin(GL_QUADS);
		glTexCoord2f(0,0);          glVertex2f(0,0);
		glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
		glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
		glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
		glEnd();

		glUseProgramObjectARB(0);

		glActiveTextureARB(GL_TEXTURE1);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		unbind_fb();
		glEnable(GL_BLEND);

		if (relax_mode) {
			bind_fb(f_temp_fb, SimpleWidth, SimpleHeight);

			glDisable(GL_BLEND);
			glUseProgramObjectARB(iter_shader_no_corners->handle);
			glUniform1fARB(glGetUniformLocationARB(iter_shader_no_corners->handle, "w_edge"), 1.0f);
			glUniform1fARB(glGetUniformLocationARB(iter_shader_no_corners->handle, "w_center_inv"), 1.0f / -4.0f);

			glActiveTextureARB(GL_TEXTURE0);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
			set_clamp_to_edge();
			glActiveTextureARB(GL_TEXTURE1);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rhs_tex);
			set_clamp_to_edge();

			glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(0,0);
			glTexCoord2f(SimpleWidth,0); glVertex2f(SimpleWidth,0);
			glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
			glTexCoord2f(0,SimpleHeight); glVertex2f(0,SimpleHeight);
			glEnd();

			glActiveTextureARB(GL_TEXTURE1);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glActiveTextureARB(GL_TEXTURE0);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			glUseProgramObjectARB(0);
			unbind_fb();

			swap(f_temp_fb, f_fb);
			swap(f_temp_tex, f_tex);

			glEnable(GL_BLEND);
		} else {
			//--------------------------------
			//read back:
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rhs_tex);
			vector< Vector3f > rhs_temp(SimpleWidth * SimpleHeight);
			glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, GL_FLOAT, &(rhs_temp[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			//--------------------------------
			//solve:
			for (unsigned int c = 0; c < 3; ++c) {
				//read:
				for (unsigned int i = 0; i < SimpleWidth*SimpleHeight; ++i) {
					simple_rhs[i] = rhs_temp[i].c[c];
				}
				fftwf_execute(simple_forward);
				//deconvolve:
				for (unsigned int i = 0; i < SimpleWidth*(SimpleHeight/2+1); ++i) {
					simple_RHS[i][0] *= simple_INV_FILTER[i];
					simple_RHS[i][1] *= simple_INV_FILTER[i];
				}
				fftwf_execute(simple_backward);
				//write:
				for (unsigned int i = 0; i < SimpleWidth*SimpleHeight; ++i) {
					rhs_temp[i].c[c] = simple_rhs[i];
				}
			}
			//--------------------------------
			//upload:
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, SimpleWidth, SimpleHeight, 0, GL_RGB, GL_FLOAT, &(rhs_temp[0]));
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		} //direct method.
	}
	Graphics::gl_errors("FFTPaint::update");
}

bool FFTPaintModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_e) {
		freq_edit = !freq_edit;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_f) {
		show = SHOW_F;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_x) {
		if (event.key.keysym.mod & KMOD_SHIFT) {
			show = SHOW_GXR;
		} else {
			show = SHOW_GX;
		}
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_y) {
		if (event.key.keysym.mod & KMOD_SHIFT) {
			show = SHOW_GYR;
		} else {
			show = SHOW_GY;
		}
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
		show = SHOW_RHS;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
		load();
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		grad_dirty = true;
		vector< Vector3f > f_temp(SimpleWidth * SimpleHeight);
		for (unsigned int i = 0; i < f_temp.size(); ++i) {
			f_temp[i].x = rand() / float(RAND_MAX);
			f_temp[i].y = rand() / float(RAND_MAX) - 0.5f;
			f_temp[i].z = rand() / float(RAND_MAX) - 0.5f;
		}
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, SimpleWidth, SimpleHeight, 0, GL_RGB, GL_FLOAT, &(f_temp[0]));
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_i) {
		relax_mode = !relax_mode;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			if (freq_edit) {
				build_edit_tex();
			}
			if (clone) {
				bind_fb(freq_edit?gx_edit_fb:gx_fb, SimpleWidth, SimpleHeight);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_clone_tex);
				glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, 0, 0, SimpleWidth, SimpleHeight, 0);
				unbind_fb();
				bind_fb(freq_edit?gy_edit_fb:gy_fb, SimpleWidth, SimpleHeight);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_clone_tex);
				glCopyTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, 0, 0, SimpleWidth, SimpleHeight, 0);
				unbind_fb();
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
				clone_offset = clone_point - mouse_pos;
			}
			painting = true;
			return true;
		}
		if (event.button.button == SDL_BUTTON_RIGHT) {
			clone_point = mouse_pos;
			clone = !clone;
		}
		if (event.button.button == SDL_BUTTON_WHEELUP) {
			brush_size -= 1;
			if (brush_size < 0.5f) brush_size = 0.5f;
			return true;
		}
		if (event.button.button == SDL_BUTTON_WHEELDOWN) {
			brush_size += 1;
			return true;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		if (freq_edit) {
			collapse_edit_tex();
		}
		painting = false;
	}
	if (event.type == SDL_MOUSEMOTION) {
		Vector2f old_pos = mouse_pos;
		mouse_pos = (local_mouse + make_vector(1.0f, 1.0f)) * 0.5f;
		mouse_pos.x *= SimpleWidth;
		mouse_pos.y = (1.0f - mouse_pos.y) * SimpleHeight;
		Vector2f dir = perpendicular(normalize(mouse_pos - old_pos));
		float weight = length(mouse_pos - old_pos);
		mouse_dir += (1.0f - 1.0f / (1.0f + 0.1f * weight)) * (dir - mouse_dir);
		mouse_dir = normalize(mouse_dir);
		if (painting) {

			float R = color().x;
			float G = color().y;
			float B = color().z;
			float Y_ = Kr * R + (1 - Kr - Kb) * G + Kb * B;
			float Pb = 0.5f * (B - Y_) / (1 - Kb);
			float Pr = 0.5f * (R - Y_) / (1 - Kr);

			if (freq_edit) {
				//swap to edit gradients:
				swap(gx_tex, gx_edit_tex);
				swap(gx_fb, gx_edit_fb);
				swap(gy_tex, gy_edit_tex);
				swap(gy_fb, gy_edit_fb);
			}
			grad_line(old_pos, mouse_pos, brush_size, clone?make_vector(1.0f,1.0f):mouse_dir, clone?make_vector(1.0f, 1.0f, 1.0f, 1.0f):make_vector(Y_, Pb, Pr, 1.0f / brush_size));
			if (freq_edit) {
				//swap back to full gradients:
				swap(gx_tex, gx_edit_tex);
				swap(gx_fb, gx_edit_fb);
				swap(gy_tex, gy_edit_tex);
				swap(gy_fb, gy_edit_fb);
				collapse_edit_tex();
			}
		}
		return true;
	}
	return false;
}

void FFTPaintModule::calc_residual_gradients() {
	//------------------------------------------------------
	//Do the x-residual:
	bind_fb(gx_residual_fb, SimpleWidth, SimpleHeight);

	glUseProgramObjectARB(gx_residual_shader->handle);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	set_clamp_to_edge();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
	set_clamp_to_black();
	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0); glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight); glVertex2f(0,SimpleHeight);
	glEnd();

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();

	//------------------------------------------------------
	//Do the y-residual:
	bind_fb(gy_residual_fb, SimpleWidth, SimpleHeight);

	glUseProgramObjectARB(gy_residual_shader->handle);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	set_clamp_to_edge();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
	set_clamp_to_black();
	glDisable(GL_BLEND);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0); glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight); glVertex2f(0,SimpleHeight);
	glEnd();

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();

	glEnable(GL_BLEND);

	Graphics::gl_errors("calc_residual_gradients");
}

void FFTPaintModule::load() {
	//clear old data:
	bind_fb(gx_fb, SimpleWidth, SimpleHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	unbind_fb();
	bind_fb(gy_fb, SimpleWidth, SimpleHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	unbind_fb();
	bind_fb(f_fb, SimpleWidth, SimpleHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	unbind_fb();
	grad_dirty = true;

	//load new image:
	vector< float > r,g,b;
	unsigned int width, height;
	bool have_image = load_ppm_channels(filename, width, height, r, g, b);
	if (!have_image || width != SimpleWidth || height != SimpleHeight) {
		cerr << "Cannot load image, or image not " << SimpleWidth << " x " << SimpleHeight << endl;
		return;
	}

	vector< Vector3f > f(SimpleWidth * SimpleHeight, make_vector< float, 3 >(0));
	vector< Vector3f > gx(SimpleWidth * SimpleHeight, make_vector< float, 3 >(0));
	vector< Vector3f > gy(SimpleWidth * SimpleHeight, make_vector< float, 3 >(0));
	for (unsigned int y = 0; y < SimpleHeight; ++y) {
		for (unsigned int x = 0; x < SimpleWidth; ++x) {
			float R = r[y * SimpleWidth + x];
			float G = g[y * SimpleWidth + x];
			float B = b[y * SimpleWidth + x];
			float Y_ = Kr * R + (1 - Kr - Kb) * G + Kb * B;
			f[y * SimpleWidth + x].x = Y_;
			f[y * SimpleWidth + x].y = 0.5f * (B - Y_) / (1 - Kb);
			f[y * SimpleWidth + x].z = 0.5f * (R - Y_) / (1 - Kr);
		}
	}
	//we're doing a quick/lame black-border thing here:
	for (unsigned int y = 0; y < SimpleHeight; ++y) {
		f[y * SimpleWidth + 0] = make_vector(0.0f, 0.0f, 0.0f);
		f[y * SimpleWidth + SimpleWidth-1] = make_vector(0.0f, 0.0f, 0.0f);
	}
	for (unsigned int x = 0; x < SimpleWidth; ++x) {
		f[0 * SimpleWidth + x] = make_vector(0.0f, 0.0f, 0.0f);
		f[(SimpleHeight-1) * SimpleWidth + x] = make_vector(0.0f, 0.0f, 0.0f);
	}
	for (unsigned int y = 0; y < SimpleHeight; ++y) {
		for (unsigned int x = 0; x < SimpleWidth; ++x) {
			if (x + 1 < SimpleWidth) {
				gx[y*SimpleWidth+x] = f[y * SimpleWidth + x+1] - f[y * SimpleWidth + x];
			} else {
				gx[y*SimpleWidth+x] = make_vector(0.0f, 0.0f, 0.0f);
			}
			if (y + 1 < SimpleHeight) {
				gy[y*SimpleWidth+x] = f[(y+1) * SimpleWidth + x] - f[y * SimpleWidth + x];
			} else {
				gy[y*SimpleWidth+x] = make_vector(0.0f, 0.0f, 0.0f);
			}
		}
	}

	//Upload:
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, f_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, SimpleWidth, SimpleHeight, 0, GL_RGB, GL_FLOAT, &(f[0]));
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, SimpleWidth, SimpleHeight, 0, GL_RGB, GL_FLOAT, &(gx[0]));
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, SimpleWidth, SimpleHeight, 0, GL_RGB, GL_FLOAT, &(gy[0]));
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

}

void FFTPaintModule::grad_line(Vector2f from, Vector2f to, float radius, Vector2f dir, Vector4f c) {

	Vector2f norm = normalize(from - to);
	Vector2f perp = perpendicular(norm);

	dir *= 0.1f;

	if (clone) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_clone_tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		dir.x = dir.y = 1.0f;
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glUseProgramObjectARB(texcoord_to_color->handle);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	glDisable(GL_BLEND);

	bind_fb(gx_fb, SimpleWidth, SimpleHeight);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	#define IN glTexCoord4f(c.x * dir.x, c.y * dir.x, c.z * dir.x, c.w)
	#define OUT glTexCoord4f(c.x * dir.x, c.y * dir.x, c.z * dir.x, 0.0f)
	#define VERT( X ) if (clone) glTexCoord( (X) + clone_offset ); glVertex( X )
	glBegin(GL_TRIANGLE_STRIP);
	OUT; VERT(to - norm * radius);
	IN;  VERT(to);
	OUT; VERT(to - perp * radius);
	IN;  VERT(from);
	OUT; VERT(from - perp * radius);
	IN;  VERT(from);
	OUT; VERT(from + norm * radius);
	IN;  VERT(from);
	OUT; VERT(from + perp * radius);
	IN;  VERT(to);
	OUT; VERT(to + perp * radius);
	OUT; VERT(to - norm * radius);
	glEnd();
	#undef VERT
	#undef IN
	#undef OUT
	
	unbind_fb();

	bind_fb(gy_fb, SimpleWidth, SimpleHeight);

	if (clone) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_clone_tex);
	}

	#define IN glTexCoord4f(c.x * dir.y, c.y * dir.y, c.z * dir.y, c.w)
	#define OUT glTexCoord4f(c.x * dir.y, c.y * dir.y, c.z * dir.y, 0.0f)
	#define VERT( X ) if (clone) glTexCoord( (X) + clone_offset ); glVertex( X )
	glBegin(GL_TRIANGLE_STRIP);
	OUT; VERT(to - norm * radius);
	IN;  VERT(to);
	OUT; VERT(to - perp * radius);
	IN;  VERT(from);
	OUT; VERT(from - perp * radius);
	IN;  VERT(from);
	OUT; VERT(from + norm * radius);
	IN;  VERT(from);
	OUT; VERT(from + perp * radius);
	IN;  VERT(to);
	OUT; VERT(to + perp * radius);
	OUT; VERT(to - norm * radius);
	glEnd();
	#undef VERT
	#undef IN
	#undef OUT

	unbind_fb();

	if (clone) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	} else {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgramObjectARB(0);
	}

	grad_dirty = true;
	glEnable(GL_BLEND);

	Graphics::gl_errors("line");
}

void FFTPaintModule::build_edit_tex() {
	glDisable(GL_BLEND);
	//make gx_lf:
	bind_fb(gx_lf_fb, SimpleWidth, SimpleHeight);

	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
	set_clamp_to_edge();

	glUseProgramObjectARB(lf_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	//make gy_lf:
	bind_fb(gy_lf_fb, SimpleWidth, SimpleHeight);

	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
	set_clamp_to_edge();

	glUseProgramObjectARB(lf_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	//-------------------------------------------

	//gx_edit = gx - gx_lf:
	bind_fb(gx_edit_fb, SimpleWidth, SimpleHeight);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_tex);
	set_clamp_to_black();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_lf_tex);
	set_clamp_to_black();

	glUseProgramObjectARB(subtract_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	//gy_edit = gy - gy_lf:
	bind_fb(gy_edit_fb, SimpleWidth, SimpleHeight);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_tex);
	set_clamp_to_black();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_lf_tex);
	set_clamp_to_black();

	glUseProgramObjectARB(subtract_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	glEnable(GL_BLEND);
}

void FFTPaintModule::collapse_edit_tex() {
	glDisable(GL_BLEND);
	bind_fb(gx_fb, SimpleWidth, SimpleHeight);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_edit_tex);
	set_clamp_to_black();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gx_lf_tex);
	set_clamp_to_black();

	glUseProgramObjectARB(add_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	//gy gets gy_lf+gy_edit...
	bind_fb(gy_fb, SimpleWidth, SimpleHeight);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_edit_tex);
	set_clamp_to_black();
	glActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gy_lf_tex);
	set_clamp_to_black();

	glUseProgramObjectARB(add_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);          glVertex2f(0,0);
	glTexCoord2f(SimpleWidth,0);      glVertex2f(SimpleWidth,0);
	glTexCoord2f(SimpleWidth,SimpleHeight); glVertex2f(SimpleWidth,SimpleHeight);
	glTexCoord2f(0,SimpleHeight);     glVertex2f(0,SimpleHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	glEnable(GL_BLEND);
}
