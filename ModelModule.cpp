#include "ModelModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include "shapes.hpp"

#include <sstream>

using std::cout;
using std::istringstream;

REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);

namespace {
	Module *create_modelmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new ModelModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("model", create_modelmodule);
		}
	} fred;
}

Vector2f ModelModule::size() {
	return make_vector(1.0f, 1.0f);
}

namespace {
	GLuint tex_obj = 0;
}

void ModelModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	if (tex_obj == 0) {
		glGenTextures(1, &tex_obj);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_obj);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, Game2d::bins.size(), Game2d::bins.size(), 0, GL_LUMINANCE, GL_FLOAT, &(Game2d::user_model[0]));
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex_obj);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2i(0, Game2d::bins.size());
	glVertex2f(-0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(Game2d::bins.size(), Game2d::bins.size());
	glVertex2f( 0.5f * size.x, -0.5f * size.y);
	glTexCoord2i(Game2d::bins.size(),0);
	glVertex2f( 0.5f * size.x,  0.5f * size.y);
	glTexCoord2i(0,0);
	glVertex2f(-0.5f * size.x,  0.5f * size.y);
	glEnd();
	glDisable(GL_TEXTURE_RECTANGLE_ARB);

	if (index1() < Game2d::bins.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(-0.5f * size.x, 0.5f - index1() / float(Game2d::bins.size()));
		glVertex2f( 0.5f * size.x, 0.5f - index1() / float(Game2d::bins.size()));
		glVertex2f( 0.5f * size.x, 0.5f - (index1() + 1) / float(Game2d::bins.size()));
		glVertex2f(-0.5f * size.x, 0.5f - (index1() + 1) / float(Game2d::bins.size()));
		glEnd();
	}

	if (index2() < Game2d::bins.size()) {
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 0.0f, 0.7f);
		glVertex2f(index2() / float(Game2d::bins.size()) - 0.5f, 0.5f * size.y);
		glVertex2f(index2() / float(Game2d::bins.size()) - 0.5f,-0.5f * size.y);
		glVertex2f((index2() + 1) / float(Game2d::bins.size()) - 0.5f,-0.5f * size.y);
		glVertex2f((index2() + 1) / float(Game2d::bins.size()) - 0.5f, 0.5f * size.y);
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
	Graphics::gl_errors("Model::draw");

}

void ModelModule::update(float elapsed_time) {
	if (index1() < Game2d::bins.size()) {
		index1.position = make_vector(-0.5f, 0.5f - (index1() + 0.5f) / Game2d::bins.size());
	} else {
		index1.position = make_vector(-0.5f, 0.0f);
	}
	if (index2() < Game2d::bins.size()) {
		index2.position = make_vector((index2() + 0.5f) / Game2d::bins.size() - 0.5f, -0.5f);
	} else {
		index2.position = make_vector(0.0f,-0.5f);
	}
	if (index1() < Game2d::bins.size() && index2() < Game2d::bins.size()) {
		value() = Game2d::get_transition_probability(index1(), index2());
		value.position = make_vector((index2() + 0.5f) / Game2d::bins.size() - 0.5f, 0.5f - (index1() + 0.5f) / Game2d::bins.size());
	}
}

bool ModelModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		Game2d::do_frequency_counts();
		glDeleteTextures(1, &tex_obj);
		tex_obj = 0;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_u) {
		Game2d::user_model.clear();
		Game2d::user_model.resize(Game2d::bins.size() * Game2d::bins.size(), 1.0f / Game2d::bins.size());
		glDeleteTextures(1, &tex_obj);
		tex_obj = 0;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_i) {
		Game2d::user_model.clear();
		Game2d::user_model.resize(Game2d::bins.size() * Game2d::bins.size(), 0.0f);
		for (unsigned int b = 0; b < Game2d::bins.size(); ++b) {
			Game2d::user_model.at(b * Game2d::bins.size() + b) = 1.0f;
		}
		glDeleteTextures(1, &tex_obj);
		tex_obj = 0;
		return true;
	}

	return false;
}
