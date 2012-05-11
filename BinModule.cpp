#include "BinModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <algorithm>

namespace {
	Module *create_binmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new BinModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("bin", create_binmodule);
		}
	} fred;
}

Vector2f BinModule::size() {
	return make_vector(1.5f, 1.0f);
}

void BinModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(-0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x,  0.5f * size.y, 1.0f);
	glVertex3f(-0.5f * size.x,  0.5f * size.y, 1.0f);
	glEnd();

	static vector< float > jump_bins;
	static vector< float > nojump_bins;
	static bool inited = false;
	if (!inited) {
		inited = true;
		jump_bins.clear();
		nojump_bins.clear();
		for (unsigned int b = 0; b < Game2d::bins.size(); ++b) {
			if (Game2d::bins[b].jump) {
				jump_bins.push_back(Game2d::bins[b].vel);
			} else {
				nojump_bins.push_back(Game2d::bins[b].vel);
			}
		}
		std::sort(jump_bins.begin(), jump_bins.end());
		std::sort(nojump_bins.begin(), nojump_bins.end());
	}

	const float min = -3.1f;
	const float max =  3.1f;

	if (control().jump && index() < Game2d::bins.size()) {
		for (unsigned int b = 0; b < jump_bins.size(); ++b) {
			if (jump_bins[b] == Game2d::bins[index()].vel) {
				float prev = min;
				float next = max;
				if (b > 0) prev = 0.5f * (jump_bins[b-1] + jump_bins[b]);
				if (b + 1 < jump_bins.size()) next = 0.5f * (jump_bins[b] + jump_bins[b+1]);
				prev = ((prev - min) / (max - min) - 0.5f) * size.x;
				next = ((next - min) / (max - min) - 0.5f) * size.x;
				glBegin(GL_QUADS);
				glColor3f(0.7f, 0.7f, 0.4f);
				glVertex2f(prev, 0.0f);
				glVertex2f(next, 0.0f);
				glVertex2f(next, 0.5f * size.y);
				glVertex2f(prev, 0.5f * size.y);
				glEnd();
				break;
			}
		}
	}
	if (!control().jump && index() < Game2d::bins.size()) {
		for (unsigned int b = 0; b < nojump_bins.size(); ++b) {
			if (jump_bins[b] == Game2d::bins[index()].vel) {
				float prev = min;
				float next = max;
				if (b > 0) prev = 0.5f * (nojump_bins[b-1] + nojump_bins[b]);
				if (b + 1 < nojump_bins.size()) next = 0.5f * (nojump_bins[b] + nojump_bins[b+1]);
				prev = ((prev - min) / (max - min) - 0.5f) * size.x;
				next = ((next - min) / (max - min) - 0.5f) * size.x;
				glBegin(GL_QUADS);
				glColor3f(0.7f, 0.7f, 0.4f);
				glVertex2f(prev, 0.0f);
				glVertex2f(next, 0.0f);
				glVertex2f(next,-0.5f * size.y);
				glVertex2f(prev,-0.5f * size.y);
				glEnd();
				break;
			}
		}
	}

	glBegin(GL_LINES);
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex2f(-size.x * 0.5f, 0.0f);
	glVertex2f( size.x * 0.5f, 0.0f);
	for (unsigned int b = 1; b < jump_bins.size(); ++b) {
		float pos = (((jump_bins[b-1] + jump_bins[b]) * 0.5f - min) / (max - min) - 0.5f) * size.x;
		glVertex2f(pos, 0.0f);
		glVertex2f(pos, size.y * 0.5f);
	}
	for (unsigned int b = 1; b < nojump_bins.size(); ++b) {
		float pos = (((nojump_bins[b-1] + nojump_bins[b]) * 0.5f - min) / (max - min) - 0.5f) * size.x;
		glVertex2f(pos, 0.0f);
		glVertex2f(pos, size.y * -0.5f);
	}
	glEnd();

	glBegin(GL_LINES);
	Vector2f pos;
	pos.y = size.y * (control().jump ? 0.25f : -0.25f);
	pos.x = ((control().vel - min) / (max - min) - 0.5f) * size.x;
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex(pos + make_vector( 0.1f, 0.1f));
	glVertex(pos + make_vector(-0.1f,-0.1f));
	glVertex(pos + make_vector( 0.1f,-0.1f));
	glVertex(pos + make_vector(-0.1f, 0.1f));
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x,  0.5f * size.y, 1.0f);
	glVertex3f(-0.5f * size.x,  0.5f * size.y, 1.0f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Bin::draw");

}

void BinModule::update(float elapsed_time) {

	const float min = -3.1f;
	const float max =  3.1f;

	index() = Game2d::control_bin(control());
	control.position.y = size().y * (control().jump ? 0.25f : -0.25f);
	control.position.x = ((control().vel - min) / (max - min) - 0.5f) * size().x;
}

bool BinModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
