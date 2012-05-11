#include "GameModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_gamemodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new GameModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("game", create_gamemodule);
		}
	} fred;
}

Vector2f GameModule::size() {
	return make_vector(0.5f, 1.0f);
}

void GameModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length("Game", 0.3f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	gentium->draw("Game", make_vector(-0.5f * len, -0.15f), 0.3f);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Game::draw");

}


void GameModule::update(float elapsed_time) {
	float floor = -10.0f;
	float min = status().pos.x - Game2d::PlayerRadius;
	float max = status().pos.x + Game2d::PlayerRadius;
	if (max > 0.0f && min < (float)Game2d::tile_heights.size()) {
		unsigned int m = (unsigned int)min;
		if (m >= Game2d::tile_heights.size()) m = 0;
		unsigned int M = (unsigned int)max;
		if (M >= Game2d::tile_heights.size()) M = Game2d::tile_heights.size() - 1;
		for (unsigned int i = m; i <= M; ++i) {
			float val = Game2d::tile_heights.at(i);
			if (val > floor && val < status().pos.y + Game2d::StepHeight) floor = val;
		}
	}
	bool on_ground = false;
	bool can_go_left = true;
	bool can_go_right = true;
	if (floor >= status().pos.y && vel().y <= 0.0f) {
		on_ground = true;
	}
	unsigned int at = (unsigned)status().pos.x;
	if (at - 1 < Game2d::tile_heights.size()) {
		if (Game2d::tile_heights.at(at-1) > status().pos.y + Game2d::StepHeight) {
			can_go_left = false;
		}
	}
	if (at + 1 < Game2d::tile_heights.size()) {
		if (Game2d::tile_heights.at(at+1) > status().pos.y + Game2d::StepHeight) {
			can_go_right = false;
		}
	}

	if (on_ground) {
		status().pos.y = floor;
		vel().y = 0.0f;
		vel().x = in_control().vel;
		if (in_control().jump) {
			vel().y += 3.0f;
		}
		if (fabs(vel().x) > 0.1f && status().flip != (vel().x < 0.0f)) {
			flip_timer += elapsed_time;
			if (flip_timer > 0.2f) {
				status().flip = !status().flip;
				flip_timer = 0.0f;
			}
		} else {
			flip_timer = 0.0f;
		}
	} else {
		flip_timer = 0.0f;
		vel().y -= 5.0f * elapsed_time;
		vel().x += elapsed_time * in_control().vel / 2.0f;
	}
	if (vel().x < -3.0f) vel().x = -3.0f;
	if (vel().x >  3.0f) vel().x =  3.0f;
	if (!can_go_right && vel().x > 0.0f) vel().x = 0.0f;
	if (!can_go_left && vel().x < 0.0f) vel().x = 0.0f;
	status().pos += vel() * elapsed_time;

	out_control().vel = vel().x;
	if (status().flip) out_control().vel *= -1.0f;
	out_control().jump = !on_ground;

	if (status().pos.y < -Game2d::PlayerHeight) spawn();
}

bool GameModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}

void GameModule::spawn() {
	unsigned int tile = 0;
	if (!Game2d::spawn_locs.empty()) {
		tile = rand() % Game2d::spawn_locs.size();
	}
	assert(tile < Game2d::tile_heights.size());
	status().pos.x = tile + 0.5f;
	status().pos.y = Game2d::tile_heights[tile];
	status().flip = false;
	vel().x = 0.0f;
	vel().y = 0.0f;
}
