#ifndef ARMMODULE_HPP
#define ARMMODULE_HPP
#include "Module.hpp"

const float BaseLen = 0.75f;
const float SegLen = 0.5f;
const float Rad = 0.05f;
const Vector2f BasePos = {{{-1.0f + Rad * 1.5f, 0.0f}}};

class ArmModule : public Module {
public:
	ArmModule() {
		in_ports.push_back(&base);
		in_ports.push_back(&seg);
		in_ports.push_back(&hand);
		base() = make_vector(BasePos.x, BasePos.y, 0.0f);
		seg() = make_vector(BaseLen+BasePos.x, BasePos.y, 0.0f);
		hand() = make_vector(BaseLen+SegLen+BasePos.x, BasePos.y, 0.0f);
		base.position = make_vector(-1.0f, 1.0f / 6.0f - 0.5f);
		seg.position  = make_vector(-1.0f, 3.0f / 6.0f - 0.5f);
		hand.position = make_vector(-1.0f, 5.0f / 6.0f - 0.5f);
	}
	virtual ~ArmModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Vector3f > base;
	PortDataContainer< Vector3f > seg;
	PortDataContainer< Vector3f > hand;
};

#endif //ARMMODULE_HPP
