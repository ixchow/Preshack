#include "Animation.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <iostream>

using std::cout;

namespace {
	Module *create_animation(const std::string &params) {
		return new AnimationModule(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("anim", create_animation, "[noframe] filename[0000].png another.png");
		}
	} fred;
}

AnimationModule::AnimationModule(std::string params) {
	frame = 0;
	bad_filename = "";
	if (params.size() >= 7 && params.substr(0,7) == "noframe" && (params.size() == 7 || params.substr(7,1) == " ")) {
		no_frame = true;
		params.erase(0,7);
		if (params.size()) {
			params.erase(0,1);
		}
	} else {
		no_frame = false;
	}
	while (!params.empty()) {
		for (unsigned int i = 0; i + 4 <= params.size(); ++i) {
			if (params.substr(i,4) == ".png" && (i + 4 == params.size() || params[i+4] == ' ')) {
				filenames.push_back(params.substr(0,i+4));
				params.erase(0,i+5); //get space, if exists.
				break;
			}
		}
	}
}

Vector2f AnimationModule::size() {
	if (frame < filenames.size() && filenames[frame] != bad_filename) {
		Graphics::TextureRef tex = Graphics::get_texture(filenames[frame], false, false);
		if (tex.ref) {
			return make_vector(tex->w / float(tex->h), 1.0f);
		} else {
			bad_filename = filenames[frame];
		}
	}
	return make_vector(1.0f, 1.0f);
}

void AnimationModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f sz = size();

	if (frame < filenames.size() && filenames[frame] != bad_filename) {
		Graphics::TextureRef tex = Graphics::get_texture(filenames[frame], false, false);
		if (tex.ref) {
			glBindTexture(GL_TEXTURE_2D, tex->obj);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		} else {
			bad_filename = filenames[frame];
			glBegin(GL_QUADS);
			glColor3f(0.0f, 0.0f, 0.0f);
			glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
			glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
			glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
			glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
			glEnd();
		}
	}

	if (!no_frame) {
		glBegin(GL_LINE_LOOP);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
		glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
		glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
		glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
		glEnd();
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Animation::draw");

}

bool AnimationModule::get_pixel_size(Vector2ui &into) {
	if (frame < filenames.size()) {
		Graphics::TextureRef tex = Graphics::get_texture(filenames[frame], false, false);
		if (tex.ref) {
			into.x = tex->w;
			into.y = tex->h;
			return true;
		}
	}
	into = make_vector(0U, 0U);
	return false;
}


void AnimationModule::update(float elapsed_time) {
}

bool AnimationModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (filenames.size()) {
			if (event.button.button == SDL_BUTTON_LEFT) {
				frame = frame + 1;
				if (frame >= filenames.size()) frame = filenames.size() - 1;
				return true;
			}
			if (event.button.button == SDL_BUTTON_RIGHT) {
				if (frame > 0) frame = frame - 1;
				return true;
			}
		}
	}
	return false;
}
