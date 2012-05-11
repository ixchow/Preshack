#include "PhysicsModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_physicsmodule(const std::string &params) {
		return new PhysicsModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("physics", create_physicsmodule);
		}
	} fred;
}

Vector2f PhysicsModule::size() {
	return make_vector(0.5f, 1.0f);
}

void PhysicsModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
	float len = gentium->get_length("Physics", 0.3f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	gentium->draw("Physics", make_vector(-0.5f * len, -0.15f), 0.3f);
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
	Graphics::gl_errors("Physics::draw");

}


const float Rad = 0.05f;
const float Time = 0.1f;
void PhysicsModule::update(float elapsed_time) {
	coords().clear();
	Vector3f p = pos();
	Vector3f v = vel();
	for (unsigned int i = 0; i < 40; ++i) {
		coords().push_back(p);
		v.y -= 1.0f * Time;
		p += v * Time;
		if (p.y < -0.5f + Rad) {
			p.y = 2.0f * (-0.5f + Rad) - p.y;
			if (v.y < 0.0f) {
				v.y = -0.9f * v.y;
			}
		}
	}
}

bool PhysicsModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
