#include "CurveModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

template< typename T >
inline T bezier_point(T const *seg, float const t) {
	return (1-t)*(1-t)*(1-t) * seg[0]
			+ 3*(1-t)*(1-t)*t * seg[1]
			+ 3*(1-t)*t*t * seg[2]
			+ t*t*t * seg[3];
}

namespace {
	Module *create_curvemodule(const std::string &params) {
		return new CurveModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("curve", create_curvemodule);
		}
	} fred;
}

Vector2f CurveModule::size() {
	return make_vector(2.0f, 1.0f);
}

void CurveModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	glColor3f(0.4f, 0.4f, 0.4f);
	glBegin(GL_QUAD_STRIP);
	for (unsigned int p = 0; p < coords().size(); ++p) {
		glVertex(coords()[p].xy);
		glVertex(coords()[p].xy + 0.05f * make_vector(cosf(coords()[p].z), sinf(coords()[p].z)));
	}
	glEnd();

	glColor3f(0.7f, 0.7f, 0.7f);
	glBegin(GL_LINE_STRIP);
	for (unsigned int p = 0; p < coords().size(); ++p) {
		glVertex(coords()[p].xy);
	}
	glEnd();

	glBegin(GL_LINES);
	for (unsigned int p = 0; p < points().size(); ++p) {
		if (p % 3 != 1 && p + 1 < points().size()) {
			glColor3f(0.6f, 0.5f, 0.5f);
			glVertex(points()[p].xy);
			glVertex(points()[p+1].xy);
		}
		glColor3f(1.0f, 0.9f, 0.9f);
		glVertex(points()[p].xy);
		glVertex(points()[p].xy + 0.05f * make_vector(cosf(points()[p].z), sinf(points()[p].z)));
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
	Graphics::gl_errors("Curve::draw");

}

void CurveModule::update(float elapsed_time) {
}

bool CurveModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !(SDL_GetModState() & KMOD_SHIFT)) {
		float base = 0.0f;
		if (points().size()) base = points().back().w + 0.1f;
		points().push_back(make_vector(local_mouse.x, local_mouse.y, 0.0f, base));
		current = points().size() - 1;
		twist = true;
		recalc();
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && (SDL_GetModState() & KMOD_SHIFT)) {
		current = (unsigned)-1;
		float dis = 0.0f;
		for (unsigned int p = 0; p < points().size(); ++p) {
			float test = length_squared(points()[p].xy - local_mouse);
			if (test < dis || current >= points().size()) {
				current = p;
				dis = test;
			}
		}
		if (dis > 0.05f * 0.05f) current = (unsigned)-1;
		twist = (event.button.button == SDL_BUTTON_RIGHT);
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && current < points().size()) {
		if (twist) {
			float ang = atan2(local_mouse.y - points()[current].y, local_mouse.x - points()[current].x);
			while (points()[current].z - float(M_PI) > ang) ang += 2.0f * float(M_PI);
			while (points()[current].z + float(M_PI) < ang) ang -= 2.0f * float(M_PI);
			points()[current].z = ang;
		} else {
			points()[current].xy = local_mouse;
		}
		recalc();
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		current = (unsigned)-1;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
		points().clear();
		recalc();
		return true;
	}
	return false;
}

void CurveModule::recalc() {
	coords().clear();
	if (points().empty()) return;
	unsigned int pos = 0;
	for (unsigned int sample = 0; sample < 40; ++sample) {
		float t = sample / float(40);
		while( pos + 3 < points().size() && points()[pos + 3].w < t) pos += 3;
		if (pos + 3 >= points().size()) break;
		float at = 0.0f;
		if (points()[pos+3].w - points()[pos].w > 0.0f) {
			at = (t - points()[pos].w) / (points()[pos+3].w - points()[pos].w);
		}
		coords().push_back(bezier_point(&(points()[pos]), at).xyz);
	}
}
