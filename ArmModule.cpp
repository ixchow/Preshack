#include "ArmModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_armmodule(const std::string &params) {
		return new ArmModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("arm", create_armmodule);
		}
	} fred;
}

Vector2f ArmModule::size() {
	return make_vector(2.0f, 1.0f);
}

#define SQRT2_2 0.70710678118654752440084436210484903928f

void capsule(float len, float mod = 1.0f) {
	float rad = Rad * mod;
	glBegin(GL_QUAD_STRIP);
	glVertex2f(-rad, 0.0f);
	glVertex2f(-rad, 0.0f);
	glVertex2f(-rad * SQRT2_2, rad * SQRT2_2);
	glVertex2f(-rad * SQRT2_2,-rad * SQRT2_2);
	glVertex2f( 0.0f, rad);
	glVertex2f( 0.0f,-rad);
	glVertex2f( len, rad);
	glVertex2f( len,-rad);
	glVertex2f(len + rad * SQRT2_2, rad * SQRT2_2);
	glVertex2f(len + rad * SQRT2_2,-rad * SQRT2_2);
	glVertex2f(len + rad, 0.0f);
	glVertex2f(len + rad, 0.0f);
	glEnd();
}

void ArmModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-1.0f, -0.5f);
	glVertex2f( 1.0f, -0.5f);
	glVertex2f( 1.0f,  0.5f);
	glVertex2f(-1.0f,  0.5f);
	glEnd();

	{ //draw arm.
		glMatrixMode(GL_MODELVIEW);

		glColor3f(0.5f, 0.5f, 0.5f);
		glPushMatrix();
		glTranslatef(base().x, base().y, 0.0f);
		glRotatef(base().z / M_PI * 180.0f,   0.0f, 0.0f, 1.0f);
		capsule(BaseLen, 0.9f);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(hand().x, hand().y, 0.0f);
		glRotatef(hand().z / M_PI * 180.0f,   0.0f, 0.0f, 1.0f);
		glBegin(GL_QUADS);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(1.5f * Rad, 1.5f * Rad);
		glVertex2f(3.0f * Rad, 0.0f);
		glVertex2f(1.5f * Rad,-1.5f * Rad);
		glEnd();
		glPopMatrix();

		glColor3f(0.7f, 0.7f, 0.7f);
		glPushMatrix();
		glTranslatef(seg().x, seg().y, 0.0f);
		glRotatef(seg().z / M_PI * 180.0f,   0.0f, 0.0f, 1.0f);
		capsule(SegLen);
		glPopMatrix();


		glMatrixMode(GL_PROJECTION);
	}

	glBegin(GL_LINE_LOOP);
	glVertex2f(-1.0f, -0.5f);
	glVertex2f( 1.0f, -0.5f);
	glVertex2f( 1.0f,  0.5f);
	glVertex2f(-1.0f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Arm::draw");

}

void ArmModule::update(float elapsed_time) {
}

bool ArmModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
