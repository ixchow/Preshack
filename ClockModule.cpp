#include "ClockModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_clockmodule(const std::string &params) {
		return new ClockModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("clock", create_clockmodule);
		}
	} fred;
}

Vector2f ClockModule::size() {
	return make_vector(0.5f, 0.5f);
}

void ClockModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.25f, -0.25f);
	glVertex2f( 0.25f, -0.25f);
	glVertex2f( 0.25f,  0.25f);
	glVertex2f(-0.25f,  0.25f);
	glEnd();

	glColor3f(0.7f, 0.7f, 0.7f);
	glBegin(GL_LINE_LOOP);
	for (unsigned int a = 0; a < 23; ++a) {
		float ang = a / float(23) * float(M_PI) * 2.0f;
		glVertex2f(0.22f * cosf(ang), 0.22f * sinf(ang));
	}
	glEnd();
	glBegin(GL_LINES);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(0.2f * cosf(M_PI * 0.5f - accum() * float(M_PI) * 2.0f), 0.2f * sinf(M_PI * 0.5f - accum() * float(M_PI) * 2.0f));
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.25f, -0.25f);
	glVertex2f( 0.25f, -0.25f);
	glVertex2f( 0.25f,  0.25f);
	glVertex2f(-0.25f,  0.25f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Clock::draw");

}

void ClockModule::update(float elapsed_time) {
	if (!paused) {
		accum() += elapsed_time;
	}
}

bool ClockModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (paused) {
			paused = false;
			accum() = 0.0f;
		} else {
			paused = true;
		}
		return true;
	}
	return false;
}
