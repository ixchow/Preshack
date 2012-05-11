#include "TransitionModule.hpp"

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
	Module *create_transitionmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new TransitionModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("transition", create_transitionmodule);
		}
	} fred;
}

Vector2f TransitionModule::size() {
	return make_vector(1.0f, 1.0f);
}

void TransitionModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	static GLuint obj = 0;
	if (obj == 0) {
		glGenTextures(1, &obj);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, obj);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, Game2d::fragments.size(), Game2d::fragments.size(), 0, GL_LUMINANCE, GL_FLOAT, &(Game2d::transition_quality[0]));
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, obj);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2i(0, Game2d::fragments.size());
	glVertex2f(-0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(Game2d::fragments.size(), Game2d::fragments.size());
	glVertex2f( 0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(Game2d::fragments.size(),0);
	glVertex2f( 0.5f * size.x,  0.5f * size.y);
	glTexCoord2i(0,0);
	glVertex2f(-0.5f * size.x,  0.5f * size.y);
	glEnd();
	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	if (index1() < Game2d::fragments.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(-0.5f * size.x, 0.5f - index1() / float(Game2d::fragments.size()));
		glVertex2f( 0.5f * size.x, 0.5f - index1() / float(Game2d::fragments.size()));
		glVertex2f( 0.5f * size.x, 0.5f - (index1() + 1) / float(Game2d::fragments.size()));
		glVertex2f(-0.5f * size.x, 0.5f - (index1() + 1) / float(Game2d::fragments.size()));
		glEnd();
	}

	if (index2() < Game2d::fragments.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(index2() / float(Game2d::fragments.size()) - 0.5f, 0.5f * size.y);
		glVertex2f(index2() / float(Game2d::fragments.size()) - 0.5f,-0.5f * size.y);
		glVertex2f((index2() + 1) / float(Game2d::fragments.size()) - 0.5f,-0.5f * size.y);
		glVertex2f((index2() + 1) / float(Game2d::fragments.size()) - 0.5f, 0.5f * size.y);
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
	Graphics::gl_errors("Transition::draw");

}

void TransitionModule::update(float elapsed_time) {
	if (index1() < Game2d::fragments.size()) {
		index1.position = make_vector(-0.5f, 0.5f - (index1() + 0.5f) / Game2d::fragments.size());
	} else {
		index1.position = make_vector(-0.5f, 0.0f);
	}
	if (index2() < Game2d::fragments.size()) {
		index2.position = make_vector((index2() + 0.5f) / Game2d::fragments.size() - 0.5f, -0.5f);
	} else {
		index2.position = make_vector(0.0f,-0.5f);
	}
	if (index1() < Game2d::fragments.size() && index2() < Game2d::fragments.size()) {
		value() = Game2d::get_transition_quality(index1(), index2());
		value.position = make_vector((index2() + 0.5f) / Game2d::fragments.size() - 0.5f, 0.5f - (index1() + 0.5f) / Game2d::fragments.size());
	}
}

bool TransitionModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
