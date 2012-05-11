#include "RewardModule.hpp"
#include "PolicyModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <Library/Library.hpp>
#include <Character/Draw.hpp>
#include <Character/Skin.hpp>
#include <Character/skin_utils.hpp>

#include "shapes.hpp"

#include <sstream>

using std::cout;
using std::istringstream;

REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);

namespace {
	Module *create_rewardmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new RewardModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("reward", create_rewardmodule);
		}
	} fred;
}

Vector2f RewardModule::size() {
	return make_vector(1.0f, Game2d::bins.size() / float(Game2d::fragments.size()));
}

namespace {
	GLuint tex_obj = 0;
}

void RewardModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	if (tex_obj == 0) {
		glGenTextures(1, &tex_obj);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_obj);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		vector< float > reward = Game2d::reward;
		float max = 1.0f;
		for (unsigned int r = 0; r < reward.size(); ++r) {
			if (reward[r] > max) max = reward[r];
		}
		cout << max << endl;
		float fac = 1.0f / max;
		for (unsigned int r = 0; r < reward.size(); ++r) {
			reward[r] *= fac;
		}
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, Game2d::bins.size(), Game2d::fragments.size(), 0, GL_LUMINANCE, GL_FLOAT, &(reward[0]));
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_obj);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	//note: texture inds are transposed!
	glTexCoord2i(0, Game2d::fragments.size());
	glVertex2f(-0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(0,0);
	glVertex2f( 0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(Game2d::bins.size(),0);
	glVertex2f( 0.5f * size.x,  0.5f * size.y);
	glTexCoord2i(Game2d::bins.size(), Game2d::fragments.size());
	glVertex2f(-0.5f * size.x,  0.5f * size.y);
	glEnd();
	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	if (frag() < Game2d::fragments.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(size.x * (0.5f - frag() / float(Game2d::fragments.size())), -0.5f * size.y);
		glVertex2f(size.x * (0.5f - frag() / float(Game2d::fragments.size())),  0.5f * size.y);
		glVertex2f(size.x * (0.5f - (frag() + 1) / float(Game2d::fragments.size())),  0.5f * size.y);
		glVertex2f(size.x * (0.5f - (frag() + 1) / float(Game2d::fragments.size())), -0.5f * size.y);
		glEnd();
	}

	if (bin() < Game2d::bins.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(0.5f * size.x, size.y * (bin() / float(Game2d::bins.size()) - 0.5f));
		glVertex2f(-0.5f * size.x, size.y * (bin() / float(Game2d::bins.size()) - 0.5f));
		glVertex2f(-0.5f * size.x, size.y * ((bin() + 1) / float(Game2d::bins.size()) - 0.5f));
		glVertex2f(0.5f * size.x, size.y * ((bin() + 1) / float(Game2d::bins.size()) - 0.5f));
		glEnd();
	}


	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.5f * size.x, -0.5f * size.y);
	glVertex2f( 0.5f * size.x, -0.5f * size.y);
	glVertex2f( 0.5f * size.x,  0.5f * size.y);
	glVertex2f(-0.5f * size.x,  0.5f * size.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Reward::draw");

}

void RewardModule::update(float elapsed_time) {
	if (frag() < Game2d::fragments.size()) {
		frag.position = make_vector(size().x * (0.5f - (frag() + 0.5f) / Game2d::fragments.size()), -0.5f * size().y);
	} else {
		frag.position = make_vector(0.0f, -0.5f * size().y);
	}
	if (bin() < Game2d::bins.size()) {
		bin.position = make_vector(-0.5f * size().x, ((bin() + 0.5f) / Game2d::bins.size() - 0.5f) * size().y);
	} else {
		bin.position = make_vector(-0.5f * size().x, 0.0f * size().y);
	}
	if (frag() < Game2d::fragments.size() && bin() < Game2d::bins.size()) {
		value() = Game2d::get_control_quality(frag(), bin());
		value.position = make_vector(size().x * (0.5f - (frag() + 0.5f) / Game2d::fragments.size()), ((bin() + 0.5f) / Game2d::bins.size() - 0.5f) * size().y);
	}
}

bool RewardModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_RIGHT) {
			Game2d::policy.clear();
			Game2d::policy.resize(Game2d::bins.size() * Game2d::fragments.size(), 0);
			clear_policy_texture();
			Game2d::reward.clear();
			Game2d::reward.resize(Game2d::bins.size() * Game2d::fragments.size(), 0.0f);
			glDeleteTextures(1,&tex_obj);
			tex_obj = 0;
			return true;
		} else if (event.button.button == SDL_BUTTON_LEFT) {
			Game2d::policy.clear();
			Game2d::policy.resize(Game2d::bins.size() * Game2d::fragments.size(), 0);
			clear_policy_texture();
			Game2d::step_iteration();
			glDeleteTextures(1,&tex_obj);
			tex_obj = 0;
			return true;
		}
	}
	return false;
}
