#include "ColorModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <iostream>

using std::istringstream;

namespace {
	Module *create_module(const std::string &params) {
		ColorModule *p = new ColorModule();
		istringstream ps(params);
		Vector3f col;
		if (ps >> col) p->set_color(col);
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("color", create_module);
		}
	} fred;
}

Vector2f ColorModule::size() {
	return make_vector(2.0f, 2.0f);
}

//as per http://www.cs.rit.edu/~ncs/color/t_convert.html
Vector3f rgb_to_hsv( Vector3f rgb ) {
	float r = rgb.x;
	float g = rgb.y;
	float b = rgb.z;

	Vector3f ret = make_vector(0.0f, 0.0f, 0.0f);
	float *h = &ret.x;
	float *s = &ret.y;
	float *v = &ret.z;

	float min, max, delta;

	min = r;
	if (g < min) min = g;
	if (b < min) min = b;
	max = r;
	if (g > max) max = g;
	if (b > max) max = b;
	*v = max;				// v

	delta = max - min;

	if( max != 0.0f )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = 0.0f;
		return ret;
	}

	if (delta != 0.0f) {
		if( r == max )
			*h = ( g - b ) / delta;		// between yellow & magenta
		else if( g == max )
			*h = 2 + ( b - r ) / delta;	// between cyan & yellow
		else
			*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	}

	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
	*h /= 360.0f;
	return ret;
}

Vector3f hsv_to_rgb( Vector3f hsv ) {

	float h = hsv.x * 360.0f;
	if (h >= 360.0f) h -= 360.0f;
	if (h < 0.0f) h += 360.0f;
	float s = hsv.y;
	float v = hsv.z;

	Vector3f ret = make_vector(0.0f, 0.0f, 0.0f);
	float *r = &ret.x;
	float *g = &ret.y;
	float *b = &ret.z;

	int i;
	float f, p, q, t;

	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return ret;
	}


	h /= 60;			// sector 0 to 5
	i = (int)h;
	if (i == 6) {
		i = 5; //avoid weird wrapping problems.
	}
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
	return ret;
}

const float TriangleRad = 0.83f;
const float CircleRad = 0.99f;

void ColorModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	//---------------------------------------
	const unsigned int Angles = 100;
	//circle:
	static vector< Vector2f > circle_verts;
	static vector< Vector3f > circle_colors;
	if (circle_verts.empty()) {
		circle_verts.resize(Angles * 2 + 2);
		circle_colors.resize(Angles * 2 + 2);
		for (unsigned int a = 0; a <= Angles; ++a) {
			float ang = a / float(Angles) * float(M_PI) * 2.0f;

			circle_colors[2*a+0] = hsv_to_rgb(make_vector(a / float(Angles), 1.0f, 1.0f));
			circle_colors[2*a+1] = hsv_to_rgb(make_vector(a / float(Angles), 1.0f, 1.0f));
			circle_verts[2*a+0] = CircleRad * make_vector(cosf(ang), sinf(ang));
			circle_verts[2*a+1] = TriangleRad * make_vector(cosf(ang), sinf(ang));
		}
	}
	assert(circle_colors.size() == circle_verts.size());

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &circle_verts[0]);
	glColorPointer(3, GL_FLOAT, sizeof(Vector3f), &circle_colors[0]);

	glDrawArrays(GL_QUAD_STRIP, 0, circle_verts.size());

	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f)*2, &circle_verts[0]);
	glColorPointer(3, GL_FLOAT, sizeof(Vector3f)*2, &circle_colors[0]);

	glDrawArrays(GL_LINE_STRIP, 0, circle_verts.size() / 2);

	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f)*2, &circle_verts[1]);
	glColorPointer(3, GL_FLOAT, sizeof(Vector3f)*2, &circle_colors[1]);

	glDrawArrays(GL_LINE_STRIP, 0, circle_verts.size() / 2);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	//triangle:
	{
		float ang = float(M_PI) * 2.0f * hsv_color.x;
		float amt = float(M_PI) * 2.0f / 3.0f;
		glBegin(GL_TRIANGLES);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 1.0f, 1.0f)));
		glVertex2f(cosf(ang) * TriangleRad, sinf(ang) * TriangleRad);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 1.0f, 0.0f)));
		glVertex2f(cosf(ang-amt) * TriangleRad, sinf(ang-amt) * TriangleRad);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 0.0f, 1.0f)));
		glVertex2f(cosf(ang+amt) * TriangleRad, sinf(ang+amt) * TriangleRad);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 1.0f, 1.0f)));
		glVertex2f(cosf(ang) * TriangleRad, sinf(ang) * TriangleRad);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 1.0f, 0.0f)));
		glVertex2f(cosf(ang-amt) * TriangleRad, sinf(ang-amt) * TriangleRad);
		glColor(hsv_to_rgb(make_vector(hsv_color.x, 0.0f, 1.0f)));
		glVertex2f(cosf(ang+amt) * TriangleRad, sinf(ang+amt) * TriangleRad);
		glEnd();

		Vector2f val_dir = -make_vector(cosf(ang-amt), sinf(ang-amt));

		Vector2f pos = val_dir * (1.5f * hsv_color.z - 1.0) * TriangleRad;
		pos += perpendicular(val_dir) * (0.5 - hsv_color.y) * sqrtf(3.0) * TriangleRad * hsv_color.z;

		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex(pos + make_vector(0.1f, 0.1f));
		glVertex(pos - make_vector(0.1f, 0.1f));
		glVertex(pos + make_vector(0.1f,-0.1f));
		glVertex(pos - make_vector(0.1f,-0.1f));
		glEnd();
	}
	//---------------------------------------

	glBegin(GL_LINE_LOOP);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f( 1.0f, -1.0f);
	glVertex2f( 1.0f,  1.0f);
	glVertex2f(-1.0f,  1.0f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Color::draw");

}

void ColorModule::update(float elapsed_time) {
}

bool ColorModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		dragging_triangle = (length_squared(local_mouse) < TriangleRad * TriangleRad);
		handle_position(local_mouse);
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && event.motion.state) {
		handle_position(local_mouse);
		return true;
	}
	return false;
}

void ColorModule::set_color(Vector3f rgb_color) {
	hsv_color = rgb_to_hsv(rgb_color);
	color() = hsv_to_rgb(hsv_color);
}

void ColorModule::handle_position(Vector2f pos) {
	if (dragging_triangle) {
		float ang = float(M_PI) * 2.0f * hsv_color.x;
		float amt = float(M_PI) * 2.0f / 3.0f;

		Vector2f val_dir = -make_vector(cosf(ang-amt), sinf(ang-amt));

		hsv_color.z = (pos * val_dir / TriangleRad + 1.0) / 1.5;
		hsv_color.y = (pos * perpendicular(val_dir));
		if (hsv_color.z <= 0.0f) {
			hsv_color.z = 0.0f;
			hsv_color.y = 0.5f;
		} else {
			hsv_color.y /= hsv_color.z * TriangleRad * sqrtf(3.0f);
			hsv_color.y = 0.5f - hsv_color.y;
			if (hsv_color.y < 0.0f) hsv_color.y = 0.0f;
			if (hsv_color.y > 1.0f) hsv_color.y = 1.0f;
			if (hsv_color.z > 1.0f) hsv_color.z = 1.0f;
		}

	} else {
		float ang = atan2(pos.y, pos.x);
		hsv_color.x = ang / (2.0f * float(M_PI));
		if (hsv_color.x < 0.0f) hsv_color.x += 1.0f;
	}
	color() = hsv_to_rgb(hsv_color);
}
