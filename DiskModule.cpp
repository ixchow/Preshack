#include "DiskModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>

using std::istringstream;

namespace {
	Module *create_module(const std::string &params) {
		DiskModule *p = new DiskModule();
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("disk", create_module);
		}
	} fred;
}

Vector2f DiskModule::size() {
	return make_vector(1.0f, 1.0f);
}

void DiskModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.5f, -0.5f);
	glVertex2f( 0.5f, -0.5f);
	glVertex2f( 0.5f,  0.5f);
	glVertex2f(-0.5f,  0.5f);
	glEnd();

	//---------------------------------------
	const unsigned int Angles = 16;
	const float DiskRad = 0.27f;
	const float CircleRad = 0.4f;
	glColor3f(1.0f, 1.0f, 1.0f);
	if (step == 1) {
		//disk:
		glBegin(GL_TRIANGLE_FAN);
		for (unsigned int a = 0; a < Angles; ++a) {
			float ang = a / float(Angles) * float(M_PI) * 2.0f;
			glVertex2f(cosf(ang) * DiskRad, sinf(ang) * DiskRad);
		}
		glEnd();
	} else if (step == 0) {
		//circle:
		glBegin(GL_QUAD_STRIP);
		for (unsigned int a = 0; a <= Angles; ++a) {
			float ang = a / float(Angles) * float(M_PI) * 2.0f;
			glVertex2f(cosf(ang) * DiskRad, sinf(ang) * DiskRad);
			glVertex2f(cosf(ang) * CircleRad, sinf(ang) * CircleRad);
		}
		glEnd();
	}
	//---------------------------------------

	glBegin(GL_LINE_LOOP);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex2f(-0.5f, -0.5f);
	glVertex2f( 0.5f, -0.5f);
	glVertex2f( 0.5f,  0.5f);
	glVertex2f(-0.5f,  0.5f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Disk::draw");

}

void DiskModule::update(float elapsed_time) {
	if (animate) {
		step = (step + 1) % 2;
	}
}

bool DiskModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		step = (step + 1) % 3;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		animate = !animate;
		return true;
	}
	return false;
}
