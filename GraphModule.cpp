#include "GraphModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_graphmodule(const std::string &params) {
		return new GraphModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("graph", create_graphmodule);
		}
	} fred;
}

Vector2f GraphModule::size() {
	return make_vector(2.0f, 1.0f);
}

const float Time = 0.2f;
void GraphModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	if (!graph().empty()) {
		float g_min = graph()[0];
		float g_max = graph()[0];
		for (unsigned int p = 1; p < graph().size(); ++p) {
			if (graph()[p] < g_min) g_min = graph()[p];
			if (graph()[p] > g_max) g_max = graph()[p];
		}
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		float s = 1.0f;
		if (g_max != g_min) s = 0.9f / (g_max - g_min);
		glTranslatef(-1.0f, 0.0f, 0.0f);
		glScalef(2.0f / graph().size(), s, 1.0f);
		glTranslatef(0.0f, -0.5f * (g_max + g_min), 0.0f);

		glBegin(GL_LINE_STRIP);
		glColor3f(0.7f, 0.7f, 0.7f);
		for (unsigned int p = 0; p < graph().size(); ++p) {
			glVertex2f(p, graph()[p]);
		}
		glEnd();

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	}

	if (!graph().empty()) {
		glBegin(GL_LINES);
		glColor3f(0.7f, 0.2f, 0.2f);
		float t = time();
		if (graph().empty()) t = 0.0f;
		else t = fmodf(t, graph().size());
		glVertex2f(t / Time * 0.05f - 1.0f, -0.5f);
		glVertex2f(t / Time * 0.05f - 1.0f,  0.5f);
		glEnd();
	}

	glBegin(GL_LINE_LOOP);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-1.0f, -0.5f);
	glVertex2f( 1.0f, -0.5f);
	glVertex2f( 1.0f,  0.5f);
	glVertex2f(-1.0f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Graph::draw");

}

void GraphModule::update(float elapsed_time) {
	if (graph().empty()) return;
	float t = fmodf(time(), Time * graph().size());
	unsigned int frame = (unsigned int)(t / Time);
	if (frame < graph().size()) {
		current() = graph()[frame];
		if (frame + 1 < graph().size()) {
			float i = (t - float(frame) * Time) / Time;
			current() += (graph()[frame+1] - current()) * i;
		}
	}
}

bool GraphModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
