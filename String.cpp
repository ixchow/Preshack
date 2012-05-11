#include "String.hpp"
#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

Vector2f String::size() {
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	Vector2f ret;
	ret.y = 1.0f;
	ret.x = gentium->get_length(text, ret.y);
	return ret;
}

void String::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glMatrixMode(GL_MODELVIEW);
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length(text, 1.0f);
	glColor(color);
	gentium->draw(text, make_vector(-0.5f * len, -0.5f), 1.0f);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
