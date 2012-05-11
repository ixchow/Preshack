#include "JoystickModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include "shapes.hpp"

using std::cout;

namespace {

	SDL_Joystick *joystick = NULL;
	bool joystick_inited = false;

	Module *create_joystickmodule(const std::string &params) {
		if (!joystick_inited) {
			joystick_inited = true;
			SDL_InitSubSystem(SDL_INIT_JOYSTICK);
			joystick = NULL;
			int joysticks = SDL_NumJoysticks();
			if (joysticks > 0) {
				cout << "  Found " << joysticks << " joysticks." << endl;
				int found = 0;
				while (found < joysticks && SDL_JoystickOpened(found)) ++found;
				if (found < joysticks) {
					cout << "  Using joystick #" << found << ":" << endl;
					joystick = SDL_JoystickOpen(found);
					if (joystick) {
						cout << "    Name: " << SDL_JoystickName(found) << endl;
						cout << "    Axes: " << SDL_JoystickNumAxes(joystick) << endl;
						cout << "    Buttons: " << SDL_JoystickNumButtons(joystick) << endl;
						cout << "    Balls: " << SDL_JoystickNumBalls(joystick) << endl;
					} else {
						cout << "    Couldn't open!" << endl;
					}
				} else {
					cout << "  All already in use." << endl;
				}
			} else {
				cout << "  Found no joysticks." << endl;
			}
		}
		return new JoystickModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("joystick", create_joystickmodule);
		}
	} fred;
}

Vector2f JoystickModule::size() {
	return make_vector(1.0f, 1.0f);
}

void JoystickModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.5f, -0.5f);
	glVertex2f( 0.5f, -0.5f);
	glVertex2f( 0.5f,  0.5f);
	glVertex2f(-0.5f,  0.5f);
	glEnd();

	const Vector3f highlight = make_vector(0.9f, 0.9f, 0.9f);
	const Vector3f neutral = make_vector(0.6f, 0.6f, 0.6f);
	const Vector3f shade = make_vector(0.3f, 0.3f, 0.3f);

	float ang = -control().vel / 3.0f * float(M_PI) / 6.0f + float(M_PI) * 0.5f;

	const Vector2f body = make_vector(0.6f, 0.2f);

	const Vector2f stick = make_vector(0.1f, 0.65f);

	{ //stick
		Vector2f stick_start = make_vector(0.0f, -0.5f + body.y * 0.5f);
		Vector2f stick_end = make_vector(cosf(ang) * stick.y, -0.5f + body.y * 0.5f + sinf(ang) * stick.y);
		Vector2f perp = perpendicular(stick_end - stick_start) * (stick.x / stick.y) * 0.5f;
		glBegin(GL_QUADS);
		glColor(neutral);
		glVertex(stick_start + perp);
		glColor(shade);
		glVertex(stick_start - perp);
		glColor(neutral);
		glVertex(stick_end - perp);
		glColor(highlight);
		glVertex(stick_end + perp);
		glEnd();

		glColor(highlight);
		circle< 17 >(0.1f, stick_end);
	}

	{ //button
		const Vector2f button = make_vector(0.1f, 0.1f);
		float mod = 1.0f;
		if (control().jump) mod = 0.1f;
		glBegin(GL_QUADS);
		glColor(neutral);   glVertex2f(body.x * -0.5f + button.x * 0.1f, -0.5f + body.y);
		glColor(highlight); glVertex2f(body.x * -0.5f + button.x * 0.1f, -0.5f + body.y + button.y * mod);
		glColor(neutral);   glVertex2f(body.x * -0.5f + button.x * 1.1f, -0.5f + body.y + button.y * mod);
		glColor(shade);     glVertex2f(body.x * -0.5f + button.x * 1.1f, -0.5f + body.y);
		glEnd();
	}

	glBegin(GL_QUADS);
	glColor(neutral);   glVertex2f(body.x * -0.5f, -0.5f);
	glColor(highlight); glVertex2f(body.x * -0.5f, -0.5f + body.y);
	glColor(neutral);   glVertex2f(body.x *  0.5f, -0.5f + body.y);
	glColor(shade);     glVertex2f(body.x *  0.5f, -0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.5f, -0.5f);
	glVertex2f( 0.5f, -0.5f);
	glVertex2f( 0.5f,  0.5f);
	glVertex2f(-0.5f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Joystick::draw");

}


void JoystickModule::update(float elapsed_time) {
	if (joystick && !set) {
		SDL_JoystickUpdate();
		control().vel = 0.0f;
		if (SDL_JoystickNumAxes(joystick) > 0) {
			control().vel = (float)SDL_JoystickGetAxis(joystick, 0);
			control().vel /= 32768.0f;
			//Dead zone stuff:
			if (fabs(control().vel) < 0.1f) control().vel = 0.0f;
			else if (control().vel < 0.0f) control().vel += 0.1f;
			else if (control().vel > 0.0f) control().vel -= 0.1f;
			control().vel *= 1.0f / 0.9f;
		}
		control().vel *= 3.0f;
		control().jump = false;
		if (SDL_JoystickNumButtons(joystick) > 2) {
			control().jump = SDL_JoystickGetButton(joystick, 2);
		}
	}
}

bool JoystickModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		set = true;
		control().vel = local_mouse.x * 6.0f;
		if (control().vel < -3.0f) control().vel = -3.0f;
		if (control().vel >  3.0f) control().vel =  3.0f;
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))) {
		set = true;
		control().vel = local_mouse.x * 6.0f;
		if (control().vel < -3.0f) control().vel = -3.0f;
		if (control().vel >  3.0f) control().vel =  3.0f;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		set = true;
		control().jump = !control().jump;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		control().jump = false;
		control().vel = 0.0f;
		set = false;
		return true;
	}
	return false;
}
