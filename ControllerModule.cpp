#include "ControllerModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <iostream>

using std::cout;
using std::endl;

namespace {
	Module *create_controllermodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new ControllerModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("controller", create_controllermodule);
		}
	} fred;
}

Vector2f ControllerModule::size() {
	return make_vector(0.5f, 1.0f);
}

void ControllerModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length("Controller", 0.25f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	gentium->draw("Controller", make_vector(-0.5f * len, -0.15f), 0.25f);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Controller::draw");

}


void ControllerModule::update(float elapsed_time) {
	assert(!Game2d::fragments.empty());
	if (fragment >= Game2d::fragments.size()) {
		cout << "Invalid fragment!" << endl;
		fragment = 0;
	}
	time_acc += elapsed_time;
	while (time_acc >= Game2d::Delta) {
		fragment = Game2d::next_fragment(fragment, control_bin(control()));
		time_acc -= Game2d::Delta;
		if (fragment >= Game2d::fragments.size()) {
			cout << "Invalid fragment!" << endl;
			fragment = 0;
		}
	}
	frame().motion = Game2d::fragments[fragment].motion;
	frame().frame = (unsigned int)((time_acc / Game2d::Delta) * (Game2d::fragments[fragment].end_frame - Game2d::fragments[fragment].start_frame) + Game2d::fragments[fragment].start_frame);
}

bool ControllerModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
