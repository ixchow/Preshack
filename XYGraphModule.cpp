#include "XYGraphModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>

using std::istringstream;

using std::make_pair;

namespace {
	Module *create_module(const std::string &params) {
		XYGraphModule *m = new XYGraphModule();
		istringstream p(params);
		p >> m->min_val >> m->max_val;
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("xygraph", create_module, "(minx,y,z) (maxx,y,z)");
		}
	} fred;
}

Vector2f XYGraphModule::size() {
	return make_vector(2.0f, 2.0f);
}

void XYGraphModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	glPushAttrib(GL_SCISSOR_BIT);

	{ //enable clipping:
		int min_x = int((0.5f / Graphics::aspectf() * screen_viewport.min.x + 0.5f) * Graphics::screen_x + 0.5f);
		int max_x = int((0.5f / Graphics::aspectf() * screen_viewport.max.x + 0.5f) * Graphics::screen_x + 0.5f);
		int min_y = int((0.5f * screen_viewport.min.y + 0.5f) * Graphics::screen_y + 0.5f);
		int max_y = int((0.5f * screen_viewport.max.y + 0.5f) * Graphics::screen_y + 0.5f);
		if (min_x < 0) min_x = 0;
		if (max_x > Graphics::screen_x) max_x = Graphics::screen_x;
		if (min_y < 0) min_y = 0;
		if (max_y > Graphics::screen_y) max_y = Graphics::screen_y;
		glScissor(min_x, min_y, max_x - min_x, max_y - min_y);
	}
	glEnable(GL_SCISSOR_TEST);


	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(2.0f / (max_val.x - min_val.x), 2.0f / (max_val.y - min_val.y), 1.0f);
	glTranslatef(-0.5f * (max_val.x + min_val.x), -0.5f * (max_val.y + min_val.y), 0.0f);

	//XYGraph lines:
	glBegin(GL_LINE_STRIP);
	glColor3f(0.7f, 0.7f, 0.7f);
	for (auto p = pts.begin(); p != pts.end(); ++p) {
		glVertex2f(p->first, p->second);
	}
	glEnd();

	if (recording) {
		//current operating point:
		glBegin(GL_QUADS);
		glColor3f(0.9f, 0.5f, 0.5f);
		Vector2f s = 0.01f * (max_val.xy - min_val.xy);
		glVertex2f(x_val()-s.x, y_val()-s.y);
		glVertex2f(x_val()-s.x, y_val()+s.y);
		glVertex2f(x_val()+s.x, y_val()+s.y);
		glVertex2f(x_val()+s.x, y_val()-s.y);
		glEnd();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	//disable scissor:
	glPopAttrib();


	if (recording) {
		glBegin(GL_LINE_LOOP);
		glColor3f(0.9f, 0.9f, 0.9f);
		glVertex2f(-0.95f, -0.95f);
		glVertex2f( 0.95f, -0.95f);
		glVertex2f( 0.95f,  0.95f);
		glVertex2f(-0.95f,  0.95f);
		glEnd();
	}

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("XYGraph::draw");

}

void XYGraphModule::update(float elapsed_time) {
	if (recording) {
		if (z_val() != old_val.z) {
			pts.clear();
			old_val.z = z_val();
		}
		if (x_val() != old_val.x || y_val() != old_val.y) {
			pts.insert(make_pair(x_val(), y_val()));
			old_val.x = x_val();
			old_val.y = y_val();
		}
	}
}

bool XYGraphModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		recording = !recording;
		if (recording) pts.clear();
	}
	return false;
}
