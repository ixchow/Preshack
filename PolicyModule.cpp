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
	Module *create_policymodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new PolicyModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("policy", create_policymodule);
		}
	} fred;
}

Vector2f PolicyModule::size() {
	return make_vector(1.0f, Game2d::bins.size() / float(Game2d::fragments.size()));
}

namespace {
	GLuint tex_obj = 0;
}

void clear_policy_texture() {
	glDeleteTextures(1,&tex_obj);
	tex_obj = 0;
}

void PolicyModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	if (tex_obj == 0) {
		glGenTextures(1, &tex_obj);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_obj);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		static vector< Vector3f > motion_colors;
		if (motion_colors.size() != Library::motion_count()) {
			motion_colors.clear();
			motion_colors.resize(Library::motion_count());
			for (unsigned int m = 0; m < motion_colors.size(); ++m) {
				motion_colors[m] = normalize(make_vector( rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX)));
			}
		}
		vector< Vector3f > policy;
		policy.resize(Game2d::policy.size());
		for (unsigned int p = 0; p < policy.size(); ++p) {
			float amt = 1.0f;
			Vector3f col = make_vector(0.0f, 0.0f, 0.0f);
			if (Game2d::policy[p] < Game2d::fragments.size()) {
				Game2d::Fragment const &frag = Game2d::fragments[Game2d::policy[p]];
				if (frag.motion < motion_colors.size()) {
					Library::Motion const &m = Library::motion(frag.motion);
					amt = float(frag.end_frame) / float(m.frames());
					col = motion_colors[frag.motion];
				} else {
					cout << "Invalid Motion!" << endl;
				}
			} else {
				cout << "Invalid Fragment!" << endl;
			}
			policy[p] = col * amt;
		}
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, Game2d::bins.size(), Game2d::fragments.size(), 0, GL_RGB, GL_FLOAT, &(policy[0]));
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
	Graphics::gl_errors("Policy::draw");

}

void PolicyModule::update(float elapsed_time) {
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
		value() = Game2d::next_fragment(frag(), bin());
		value.position = make_vector(size().x * (0.5f - (frag() + 0.5f) / Game2d::fragments.size()), ((bin() + 0.5f) / Game2d::bins.size() - 0.5f) * size().y);
	}
}

bool PolicyModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		Game2d::pick_policy();
		clear_policy_texture();
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s) {
		Game2d::save_policy();
		return true;
	}
	return false;
}
