#include "SliderModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>

using std::istringstream;

namespace {
	Module *create_slidermodule(const std::string &params) {
		SliderModule *m = new SliderModule();
		istringstream p(params);
		p >> m->min >> m->max >> m->snap;
		if (!(p >> m->val())) {
			m->val() = (m->min + m->max) * 0.5f;
		}
		if (!(p >> m->width)) {
			m->width = 2.0f;
		}
		if (!(p >> m->rate)) {
			m->rate = 0.0f;
		}
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("slider", create_slidermodule, "min max [snap] [val] [width] [rate]");
		}
	} fred;
}

Vector2f SliderModule::size() {
	return make_vector(width, 0.3f);
}

void SliderModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.5f * size().x, -0.15f);
	glVertex2f( 0.5f * size().x, -0.15f);
	glVertex2f( 0.5f * size().x,  0.15f);
	glVertex2f(-0.5f * size().x,  0.15f);
	glEnd();

	glBegin(GL_LINES);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex2f(((val() - min) / (max - min) - 0.5f) * size().x, -0.15f);
	glVertex2f(((val() - min) / (max - min) - 0.5f) * size().x,  0.15f);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-0.5f * size().x, -0.15f);
	glVertex2f( 0.5f * size().x, -0.15f);
	glVertex2f( 0.5f * size().x,  0.15f);
	glVertex2f(-0.5f * size().x,  0.15f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Slider::draw");

}

void SliderModule::update(float elapsed_time) {
	if (playing) {
		val() += rate * elapsed_time;
		if (val() < min) {
			val() = min;
			playing = false;
		}
		if (val() > max) {
			val() = max;
			playing = false;
		}
	}
}

bool SliderModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if ((event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	 || (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)))) {
		val() = (local_mouse.x + 0.5f * size().x) / size().x * (max - min) + min;
		playing = false;
		if (snap != 0.0f) {
			float amt = fmodf(val(), snap);
			if (amt < -0.5f * snap) amt += snap;
			if (amt > 0.5f * snap) amt -= snap;
			val() -= amt;
		}
		if (val() < min) val() = min;
		if (val() > max) val() = max;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		playing = !playing;
		if (playing && val() == max) {
			val() = min;
		}
		return true;
	}
	return false;
}
