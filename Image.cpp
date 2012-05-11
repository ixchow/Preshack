#include "Image.hpp"
#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Vector/VectorGL.hpp>

REQUEST_GL_EXTENSION( GL_ARB_texture_non_power_of_two );

Graphics::TextureRef Image::get_texture() {
	Graphics::TextureRef texture;
	if (path != bad_path) {
		
		texture = Graphics::get_texture(path, !have_ARB_texture_non_power_of_two(), false);
		if (!texture.ref) {
			bad_path = path;
		}
	}
	return texture;
}

Vector2f Image::size() {
	Graphics::TextureRef texture = get_texture();
	Vector2f ret;
	ret.y = 1.0f;
	ret.x = 1.0f;
	if (texture.ref) {
		ret.x = ret.y * float(texture->w) / float(texture->h);
	}
	return ret;
}

void Image::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
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

	float fac = 1.0f;
	float mx = 1.0f;
	float my = 1.0f;
	if (texture.ref) {
		fac = float(texture->w) / float(texture->h);
		mx = float(texture->w) / float(texture->tw);
		my = float(texture->h) / float(texture->th);
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,0.0f); glVertex2f(-0.5f*fac,-0.5f);
	glTexCoord2f(0.0f,my); glVertex2f(-0.5f*fac, 0.5f);
	glTexCoord2f(mx,my); glVertex2f( 0.5f*fac, 0.5f);
	glTexCoord2f(mx,0.0f); glVertex2f( 0.5f*fac,-0.5f);
	glEnd();

	if (!texture.ref) {
		Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
		string error = "Cannot load '" + path + "'";
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
	Graphics::gl_errors("Image::draw");
}

bool Image::get_pixel_size(Vector2ui &into) {
	Graphics::TextureRef tex = get_texture();
	if (tex.ref) {
		into = make_vector< unsigned int >(tex->w, tex->h);
		return true;
	} else {
		into = make_vector(0U, 0U);
		return false;
	}
}
