#include "Equation.hpp"
#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <fstream>

using std::ofstream;

Graphics::TextureRef Equation::get_texture() {
	Graphics::TextureRef texture;
	if (equation == bad_equation) {
		return texture;
	}
	string path = "equations/";
	//create a path from the equation:
	unsigned int hash = 0;
	const int hashprime = 49157;
	for (unsigned int i = 0; i < equation.size(); ++i) {
		if (path.size() < 200) {
			if (isalnum(equation[i])) {
				path += equation[i];
			} else if (equation[i] == '\\') {
				path += '-';
			} else if (equation[i] == '{' || equation[i] == '[' || equation[i] == '(') {
				path += '(';
			} else if (equation[i] == '}' || equation[i] == ']' || equation[i] == ')') {
				path += ')';
			} else {
				path += '.';
			}
		}
		hash = (hash * 256 + equation[i]) % hashprime;
	}
	path += ".";
	while (hash > 0) {
		path += '0' + (hash % 10);
		hash /= 10;
	}
	path += ".eqn";
	texture = Graphics::get_texture(path + ".png", true);
	if (!texture.ref) {
		//--- no cached version of this eqn:
		{ //dump equation to file:
			ofstream eqn(path.c_str());
			eqn << equation;
		}
		//call conversion script to make a png:
		int ret = system(("perl equations/build_png.pl '" + path + "'").c_str());
		if (ret == -1) {
			std::cerr << "Calling perl to make eqn fails." << std::endl;
		}
		//try loading that one:
		texture = Graphics::get_texture(path + ".png", true);
		if (!texture.ref) {
			bad_equation = equation;
		}
	}
	return texture;
}


Vector2f Equation::size() {
	Graphics::TextureRef texture = get_texture();
	Vector2f ret;
	ret.y = 1.0f;
	ret.x = 1.0f;
	if (texture.ref) {
		ret.x = float(texture->w) / 500.0f;
		ret.y = float(texture->h) / 500.0f;
	}
	return ret;
}

void Equation::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Graphics::TextureRef texture = get_texture();
	if (texture.ref) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->obj);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	float mx = 1.0f;
	float my = 1.0f;
	if (texture.ref) {
		mx = float(texture->w) / float(texture->tw);
		my = float(texture->h) / float(texture->th);
	}

	Vector2f s = size();
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,my); glVertex2f(-0.5f*s.x,-0.5f*s.y);
	glTexCoord2f(0.0f,0.0f); glVertex2f(-0.5f*s.x, 0.5f*s.y);
	glTexCoord2f(mx,0.0f); glVertex2f( 0.5f*s.x, 0.5f*s.y);
	glTexCoord2f(mx,my); glVertex2f( 0.5f*s.x,-0.5f*s.y);
	glEnd();

	if (!texture.ref) {
		Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
		string error = "Bad Equation '" + equation + "'";
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
		gentium->draw(error, make_vector(-0.5f + 0.01f, -0.1f - 0.01f), 0.2f);
		glColor4f(1.0f, 0.0f, 0.0f, 0.9f);
		gentium->draw(error, make_vector(-0.5f, -0.1f), 0.2f);
	}

	if (texture.ref) {
		glDisable(GL_TEXTURE_2D);
	}
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Equation::draw");
}

bool Equation::get_pixel_size(Vector2ui &into) {
	Graphics::TextureRef tex = get_texture();
	if (!tex.ref) return false;
	into.x = tex->w;
	into.y = tex->h;
	return true;
}
