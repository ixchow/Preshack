#include "ProjectionModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>

using std::istringstream;

namespace {
	Module *create_projectionmodule(const std::string &params) {
		ProjectionModule *p = new ProjectionModule();
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("projection", create_projectionmodule);
		}
	} fred;
}

Vector2f ProjectionModule::size() {
	return make_vector(4.0f, 2.0f);
}

void ProjectionModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-2.0f, -1.0f);
	glVertex2f( 2.0f, -1.0f);
	glVertex2f( 2.0f,  1.0f);
	glVertex2f(-2.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.0f / 11.0f, 2.0f / 11.0f, 1.0f);
	glTranslatef(-1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-10.0f, 0.0f); glVertex2f(1.0f, 0.0f);
	glVertex2f(0.0f,-5.0f); glVertex2f(0.0f, 5.0f);

	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	glVertex2f(-near(),-4.5f); glVertex2f(-near(), 4.5f);
	if (far() < 10.0f) {
		glVertex2f(-far(),-4.5f); glVertex2f(-far(), 4.5f);
	}

	float slope = sinf(fov() * float(M_PI) / 180.0f * 0.5f) / cosf(fov() * float(M_PI) / 180.0f * 0.5f);

	glColor4f(1.0f, 0.9f, 0.9f, 0.5f);
	glVertex2f(0.0f, 0.0f); glVertex2f(-near(), slope * near());
	glVertex2f(0.0f, 0.0f); glVertex2f(-near(),-slope * near());
	glColor4f(1.0f, 0.9f, 0.9f, 0.75f);
	float d = far();
	if (d > 10.0f) {
		d = 10.0f;
	}
	glVertex2f(-near(), slope * near()); glVertex2f(-d, slope * d);
	glVertex2f(-near(),-slope * near()); glVertex2f(-d,-slope * d);


	for (unsigned int i = 0; i < points.size(); ++i) {
		glColor3f(0.0f, 0.7f, 0.5f);
		glVertex2f(points[i].x - 0.5f, points[i].y - 0.5f);
		glVertex2f(points[i].x + 0.5f, points[i].y + 0.5f);
		glVertex2f(points[i].x - 0.5f, points[i].y + 0.5f);
		glVertex2f(points[i].x + 0.5f, points[i].y - 0.5f);
	}

	glEnd();


	//set up for [projected] side:
	glLoadIdentity();
	glTranslatef(1.0f, 0.0f, 0.0f);
	//glScalef(0.1f, 0.1f, 0.1f);
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
	float a = 0;
	float b = 0;
	if (near() == 0.0f) {
	} else if (far() > 10.0f) {
		a = -1;
		b = -2 * near();
	} else if (near() != far()) {
		a = (near() + far()) / (near() - far());
		b = 2.0f * near() * far() / (near() - far());
	}
	float mat[16] = {
		a, 0.0f, 0.0f, -1.0f,
		0.0f, 1.0f/slope, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		b, 0.0f, 0.0f, 0.0f,
	};
	glMultMatrixf(mat);

	glBegin(GL_LINES);
	for (unsigned int i = 0; i < points.size(); ++i) {
		glColor3f(0.0f, 0.7f, 0.5f);
		glVertex2f(points[i].x - 0.5f, points[i].y - 0.5f);
		glVertex2f(points[i].x + 0.5f, points[i].y + 0.5f);
		glVertex2f(points[i].x - 0.5f, points[i].y + 0.5f);
		glVertex2f(points[i].x + 0.5f, points[i].y - 0.5f);
	}
	glEnd();

	{//disable clipping.
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
	}

	glLoadIdentity();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-2.0f, -1.0f);
	glVertex2f( 2.0f, -1.0f);
	glVertex2f( 2.0f,  1.0f);
	glVertex2f(-2.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Projection::draw");

}

void ProjectionModule::update(float elapsed_time) {
	near.position = make_vector(-2.0f * (1.0f + near()) / 11.0f, -1.0f);
	if (far() > 10.0f) {
		far.position = make_vector(-2.0f, -1.0f);
	} else {
		far.position = make_vector(-2.0f * (1.0f + far()) / 11.0f, -1.0f);
	}
}

bool ProjectionModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && local_mouse.x > -2.0f && local_mouse.x < 0.0f && local_mouse.y > -1.0f && local_mouse.y < 1.0f) {
		points.push_back(local_mouse * 11.0f / 2.0f - make_vector(-1.0f, 0.0f));
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		points.clear();
		return true;
	}
	return false;
}
