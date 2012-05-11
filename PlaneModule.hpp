#ifndef PLANEMODULE_HPP
#define PLANEMODULE_HPP
#include "Module.hpp"

const float BaseLen = 0.75f;
const float SegLen = 0.5f;
const float Rad = 0.05f;
const Vector2f BasePos = {{{-1.0f + Rad * 1.5f, 0.0f}}};

class PlaneModule : public Module {
public:
	PlaneModule() {
		do_rot = false;
		rot = 0.0f;
	}
	virtual ~PlaneModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	bool do_rot;
	float rot;
};

#endif //PLANEMODULE_HPP
