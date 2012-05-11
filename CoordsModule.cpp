#include "CoordsModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_coordsmodule(const std::string &params) {
		return new CoordsModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("coords", create_coordsmodule);
		}
	} fred;
}

Vector2f CoordsModule::size() {
	return make_vector(2.0f, 1.0f);
}

const float Time = 0.2f;
void CoordsModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	unsigned int frame = 0;
	if (!coords().empty()) {
		float t = fmodf(time(), Time * coords().size());
		frame = (unsigned int)(t / Time);
	}
	glBegin(GL_LINES);
	for (unsigned int p = 0; p < coords().size(); ++p) {
		if (p == frame) {
			glColor3f(1.0f, 0.5f, 0.5f);
		} else {
			glColor3f(1.0f, 1.0f, 1.0f);
		}
		Vector2f a = make_vector(cosf(coords()[p].z), sinf(coords()[p].z)) * 0.05f;
		Vector2f b = perpendicular(a);
		glVertex(coords()[p].xy + a * 1.5f);
		glVertex(coords()[p].xy - a);
		glVertex(coords()[p].xy + b);
		glVertex(coords()[p].xy - b);
	}
	glEnd();

	glBegin(GL_LINE_LOOP);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-1.0f, -0.5f);
	glVertex2f( 1.0f, -0.5f);
	glVertex2f( 1.0f,  0.5f);
	glVertex2f(-1.0f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Coords::draw");

}

void CoordsModule::update(float elapsed_time) {
	if (coords().empty()) return;
	float t = fmodf(time(), Time * coords().size());
	unsigned int frame = (unsigned int)(t / Time);
	if (frame < coords().size()) {
		current() = coords()[frame];
		if (frame + 1 < coords().size()) {
			float i = (t - float(frame) * Time) / Time;
			current() += (coords()[frame+1] - current()) * i;
		}
	}

}

bool CoordsModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		coords().push_back(make_vector(local_mouse, 0.0f));
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) && !coords().empty()) {
		coords().back().z = atan2(local_mouse.y - coords().back().y, local_mouse.x - coords().back().x);
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
		coords().clear();
		return true;
	}
	return false;
}
