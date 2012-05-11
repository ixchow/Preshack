#include "VideoModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);

namespace {
	Module *create_videomodule(const std::string &params) {
		return new VideoModule(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("video", create_videomodule);
		}
	} fred;
}

Vector2f VideoModule::size() {
	if (!stream.has_video()) {
		return make_vector(1.0f, 1.0f);
	} else {
		return make_vector(1.0f, float(stream.h) / stream.w);
	}
}

void VideoModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();

	if (!stream.has_video()) {
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(-0.5 * s.x, -0.5f * s.y);
		glVertex2f(-0.5 * s.x,  0.5f * s.y);
		glVertex2f( 0.5 * s.x,  0.5f * s.y);
		glVertex2f( 0.5 * s.x, -0.5f * s.y);
		glEnd();
		glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2f(-0.5 * s.x, -0.5f * s.y);
		glVertex2f( 0.5 * s.x,  0.5f * s.y);
		glVertex2f(-0.5 * s.x,  0.5f * s.y);
		glVertex2f( 0.5 * s.x, -0.5f * s.y);
		glEnd();
	} else {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
		if (dirty) {
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, stream.w, stream.h, 0, GL_RGB, GL_UNSIGNED_BYTE, stream.get_video_pixels());
			dirty = false;
		}
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0, stream.h);
		glVertex2f(-0.5 * s.x, -0.5f * s.y);

		glTexCoord2f(0, 0);
		glVertex2f(-0.5 * s.x,  0.5f * s.y);

		glTexCoord2f(stream.w, 0);
		glVertex2f( 0.5 * s.x,  0.5f * s.y);

		glTexCoord2f(stream.w, stream.h);
		glVertex2f( 0.5 * s.x, -0.5f * s.y);
		glEnd();
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Video::draw");

}

void VideoModule::update(float elapsed_time) {
	if (!paused) {
		target_time += elapsed_time;
	}
	unsigned int count = 0;
	while (target_time >= stream.video_time()) {
		if (!stream.advance_video()) break;
		dirty = true;
		stream.discard_audio();
		++count;
		if (count > 10) {
			target_time = stream.video_time();
			break;
		}
	}
}

bool VideoModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		paused = !paused;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		paused = true;
		if (!stream.open(path)) {
			cerr << "WARNING: video module can't open '" << path << "'." << endl;
		} else if (!stream.has_video()) {
			cerr << "WARNING: stream '" << path << "' doesn't contain video." << endl;
		}
	}
	return false;
}
