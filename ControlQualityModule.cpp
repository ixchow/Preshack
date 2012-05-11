#include "ControlQualityModule.hpp"

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
	Module *create_cqmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new ControlQualityModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("controlquality", create_cqmodule);
		}
	} fred;
}

Vector2f ControlQualityModule::size() {
	return make_vector(1.0f, Game2d::bins.size() / float(Game2d::fragments.size()));
}

void ControlQualityModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, Game2d::bins.size(), Game2d::fragments.size(), 0, GL_LUMINANCE, GL_FLOAT, &(Game2d::control_quality[0]));
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, obj);
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
	Graphics::gl_errors("ControlQuality::draw");

}

void ControlQualityModule::update(float elapsed_time) {
	if (frag() < Game2d::fragments.size()) {
		frag.position = make_vector(size().x * (0.5f - (frag() + 0.5f) / Game2d::fragments.size()), -0.5f * size().y);
	} else {
		frag.position = make_vector(0.0f, -0.5f * size().y);
	}
	if (bin() < Game2d::bins.size()) {
		bin.position = make_vector(-0.5f * size().x, ((bin() + 0.5f) / Game2d::bins.size() - 0.5f) * size().y);
	} else {
		bin.position = make_vector(-0.5f * size().x, 0.0f);
	}
	if (frag() < Game2d::fragments.size() && bin() < Game2d::bins.size()) {
		value() = Game2d::get_control_quality(frag(), bin());
		value.position = make_vector(size().x * (0.5f - (frag() + 0.5f) / Game2d::fragments.size()), ((bin() + 0.5f) / Game2d::bins.size() - 0.5f) * size().y);
	}
}

bool ControlQualityModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
