#include "TraceModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_tracemodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new TraceModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("trace", create_tracemodule);
		}
	} fred;
}

Vector2f TraceModule::size() {
	return make_vector(1.0f, 2.0f);
}

void TraceModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(-0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x,  0.5f * size.y, 1.0f);
	glVertex3f(-0.5f * size.x,  0.5f * size.y, 1.0f);
	glEnd();

	unsigned int begin = 0;
	if (Game2d::traces.size() > 100) begin = Game2d::traces.size() - 100;
	unsigned int offset = 100 - (Game2d::traces.size() - begin);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(0.0f, size.y * (((signed)offset - (signed)begin) / 100.0f - 0.5f), 0.0f);
	glScalef(0.5f * size.x / 3.1f, size.y / 100.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glColor3f(1.0f, 1.0f, 0.0f);
	for (unsigned int i = begin; i < Game2d::traces.size(); ++i) {
		glVertex2f(Game2d::traces[i].vel, i);
	}
	glEnd();
	glScalef(5.0f, 1.0f, 1.0f);
	glTranslatef(-0.5f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glColor3f(1.0f, 0.0f, 0.0f);
	for (unsigned int i = begin; i < Game2d::traces.size(); ++i) {
		glVertex2f(Game2d::traces[i].jump, i);
	}
	glEnd();
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);


	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x,  0.5f * size.y, 1.0f);
	glVertex3f(-0.5f * size.x,  0.5f * size.y, 1.0f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Trace::draw");

}

void TraceModule::update(float elapsed_time) {
	if (!paused) {
		time_acc += elapsed_time;
	}
	while (time_acc > Game2d::Delta) {
		if (Game2d::traces.size() < 1000) {
			Game2d::traces.push_back(control());
		}
		time_acc -= Game2d::Delta;
	}
}

bool TraceModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		paused = !paused;
		if (!paused) {
			time_acc = 0;
		}
		return true;
	}
	return false;
}
