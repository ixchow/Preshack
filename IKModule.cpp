#include "IKModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

namespace {
	Module *create_ikmodule(const std::string &params) {
		return new IKModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("ik", create_ikmodule);
		}
	} fred;
}

Vector2f IKModule::size() {
	return make_vector(0.5f, 1.0f);
}

void IKModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length("IK", 0.3f);
	glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
	gentium->draw("IK", make_vector(-0.5f * len, -0.15f), 0.3f);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.25f, -0.5f);
	glVertex2f( 0.25f, -0.5f);
	glVertex2f( 0.25f,  0.5f);
	glVertex2f(-0.25f,  0.5f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("IK::draw");

}

void IKModule::update(float elapsed_time) {
	Vector2f target = hand().xy;
	if (length(target - BasePos) > BaseLen + SegLen) {
		target = BasePos + normalize(target - BasePos) * (BaseLen + SegLen);
	}
	{
		float a = BaseLen;
		float b = SegLen;
		float c = length(target - BasePos);
		float val = (c * c - a * a - b * b) / (-2.0f * a * b);
		if (val < -1.0f) val = -1.0f;
		if (val > 1.0f) val = 1.0f;
		seg_ang() = float(M_PI) + acosf(val);
	}
	{
		Vector2f tip = BasePos + make_vector(BaseLen, 0.0f) + SegLen * make_vector(cosf(seg_ang()), sinf(seg_ang()));
		base_ang() = -atan2(tip.y - BasePos.y, tip.x - BasePos.x);
	}
	base_ang() += atan2(target.y - BasePos.y, target.x - BasePos.x);
	hand_ang() = hand().z - base_ang() - seg_ang();
}

bool IKModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
