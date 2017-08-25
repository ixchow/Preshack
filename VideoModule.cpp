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

VideoModule::VideoModule(std::string const &_path) : path(_path) {
	if (path.size() > 4 && path.substr(path.size()-4) == "auto") {
		paused = false;
		path.erase(path.size()-4,4);
	}

	try {
		stream.reset(new VidStream(path));
		stream->advance();
	} catch (std::runtime_error const &e) {
		cerr << "WARNING: video module can't open '" << path << "':\n" << e.what() << endl;
	}

	target_time = 0.0;
	tex = 0;
	glGenTextures(1, &tex);
	dirty = true;
	/*
	for (unsigned int i = 0; i < 10; ++i) {
		if (!stream.advance_video()) break;
		stream.discard_audio();
	}*/
}

VideoModule::~VideoModule() {
}


Vector2f VideoModule::size() {
	if (stream && stream->width > 0) {
		return make_vector(1.0f, float(stream->height) / stream->width);
	} else {
		return make_vector(1.0f, 1.0f);
	}
}

void VideoModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f s = size();

	if (!stream) {
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
		if (dirty && stream->rgb_data[0]) {
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, stream->width, stream->height, 0, GL_RGB, GL_UNSIGNED_BYTE, stream->rgb_data[0]);
			dirty = false;
		}
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0, stream->height);
		glVertex2f(-0.5 * s.x, -0.5f * s.y);

		glTexCoord2f(0, 0);
		glVertex2f(-0.5 * s.x,  0.5f * s.y);

		glTexCoord2f(stream->width, 0);
		glVertex2f( 0.5 * s.x,  0.5f * s.y);

		glTexCoord2f(stream->width, stream->height);
		glVertex2f( 0.5 * s.x, -0.5f * s.y);
		glEnd();
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Video::draw");

}

void VideoModule::update(float elapsed_time) {
	if (!stream) {
		target_time = 0.0;
		return;
	}
	if (!paused) {
		target_time += elapsed_time;
	}
	unsigned int count = 0;
	while (target_time > stream->timestamp) {
		if (!stream->advance()) break;
		dirty = true;
		++count;
		if (count > 10) {
			target_time = stream->timestamp;
			break;
		}
	}
}

bool VideoModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		paused = !paused;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		paused = true;
		target_time = 0.0;
		stream.reset();
		try {
			stream.reset(new VidStream(path));
			stream->advance();
			dirty = true;
		} catch (std::runtime_error const &e) {
			cerr << "WARNING: video module can't open '" << path << "':\n" << e.what() << endl;
		}
		return true;
	}
	return false;
}
