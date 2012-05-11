#include "BallModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_ballmodule(const std::string &params) {
		return new BallModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("ball", create_ballmodule);
		}
	} fred;
}

Vector2f BallModule::size() {
	return make_vector(2.0f, 1.0f);
}

#define SQRT2_2 0.70710678118654752440084436210484903928f

void BallModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	const float rad = 0.05f;
	Vector2f p = pos().xy;
	if (p.x < -1.0f + rad) p.x = -1.0f + rad;
	if (p.x >  1.0f - rad) p.x =  1.0f - rad;
	if (p.y < -0.5f + rad) p.y = -0.5f + rad;
	if (p.y >  0.5f - rad) p.y =  0.5f - rad;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(p.x, p.y, 0.0f);
	glRotatef(pos().z / M_PI * 180.0f,   0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.6f, 0.6f, 0.6f);
	glVertex2f(0.0f, 0.0f);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex(rad * make_vector( 1.0f, 0.0f));
	glVertex(rad * make_vector(SQRT2_2, SQRT2_2));
	glVertex(rad * make_vector( 0.0f, 1.0f));
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex(rad * make_vector( 0.0f, 1.0f));
	glVertex(rad * make_vector(-SQRT2_2, SQRT2_2));
	glVertex(rad * make_vector(-1.0f, 0.0f));
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex(rad * make_vector(-1.0f, 0.0f));
	glVertex(rad * make_vector(-SQRT2_2,-SQRT2_2));
	glVertex(rad * make_vector( 0.0f,-1.0f));
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex(rad * make_vector( 0.0f,-1.0f));
	glVertex(rad * make_vector(SQRT2_2,-SQRT2_2));
	glVertex(rad * make_vector( 1.0f, 0.0f));
	glEnd();
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	glBegin(GL_LINE_LOOP);
	glVertex2f(-1.0f, -0.5f);
	glVertex2f( 1.0f, -0.5f);
	glVertex2f( 1.0f,  0.5f);
	glVertex2f(-1.0f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Box::draw");

}

void BallModule::update(float elapsed_time) {
}

bool BallModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
