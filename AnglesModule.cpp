#include "AnglesModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_anglesmodule(const std::string &params) {
		return new AnglesModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("angles", create_anglesmodule);
		}
	} fred;
}

Vector2f AnglesModule::size() {
	return make_vector(0.5f, 1.0f);
}

void AnglesModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
	float len = gentium->get_length("Angles", 0.3f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	gentium->draw("Angles", make_vector(-0.5f * len, -0.15f), 0.3f);
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
	Graphics::gl_errors("Angles::draw");

}

void AnglesModule::update(float elapsed_time) {
	base().xy = BasePos;
	base().z = base_ang();
	seg().xy = base().xy + BaseLen * make_vector(cosf(base().z), sinf(base().z));
	seg().z = base_ang() + seg_ang();
	hand().xy = seg().xy + SegLen * make_vector(cosf(seg().z), sinf(seg().z));
	hand().z = base_ang() + seg_ang() + hand_ang();
}

bool AnglesModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
