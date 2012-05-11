#include "PlaneModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>

using std::istringstream;

namespace {
	Module *create_projectionmodule(const std::string &params) {
		PlaneModule *p = new PlaneModule();
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("plane", create_projectionmodule);
		}
	} fred;
}

Vector2f PlaneModule::size() {
	return make_vector(2.0f, 2.0f);
}

void PlaneModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	//set up for [projected] side:
	glLoadIdentity();


	{ //set up clipping:
		double eqn1[4] = { 1, 0, 0, 1};
		double eqn2[4] = {-1, 0, 0, 1};
		double eqn3[4] = { 0, 1, 0, 1};
		double eqn4[4] = { 0,-1, 0, 1};
		glClipPlane(GL_CLIP_PLANE0, eqn1);
		glClipPlane(GL_CLIP_PLANE1, eqn2);
		glClipPlane(GL_CLIP_PLANE2, eqn3);
		glClipPlane(GL_CLIP_PLANE3, eqn4);
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
		glEnable(GL_CLIP_PLANE2);
		glEnable(GL_CLIP_PLANE3);
	}


	float slope = sinf(0.25f * M_PI) / cosf(0.25f * M_PI);
	float a = -1;
	float b = -2 * 0.5f;
	float mat[16] = {
		1.0f/slope, 0.0f,       0.0f,  0.0f,
		0.0f,       1.0f/slope, 0.0f,  0.0f,
		0.0f,       0.0f,          a, -1.0f,
		0.0f,       0.0f,          b,  0.0f,
	};
	glMultMatrixf(mat);

	glRotatef(rot * -90.0f,  1,0,0);
	glTranslatef(0.0f, 0.0f, -1.0f);
	glScalef(10.0f, 10.0f, 1.0f);
	glTranslatef(-5.0f, -5.0f, 0.0f);

	Graphics::TextureRef tex = Graphics::get_texture("infty/checker.png");
	if (tex.ref) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex->obj);
	}

	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	for (unsigned int x = 0; x < 10; ++x) {
		for (unsigned int z = 0; z < 10; ++z) {
			glTexCoord2f(0.0f, 0.0f);
			glVertex2i(x, z);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2i(x, z+1);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2i(x+1, z+1);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2i(x+1, z);
		}
	}
	glEnd();

	if (tex.ref) {
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	{//disable clipping.
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
	}

	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Plane::draw");

}

void PlaneModule::update(float elapsed_time) {
	if (do_rot) {
		rot += elapsed_time * 0.5f;
		if (rot > 1.0f) rot = 1.0f;
	} else {
		rot -= elapsed_time * 0.5f;
		if (rot < 0.0f) rot = 0.0f;
	}
}

bool PlaneModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		do_rot = !do_rot;
		return true;
	}
	return false;
}
