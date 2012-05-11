#ifndef BALLMODULE_HPP
#define BALLMODULE_HPP
#include "Module.hpp"

class BallModule : public Module {
public:
	BallModule() {
		in_ports.push_back(&pos);
		pos() = make_vector(0.0f, 0.0f, 0.0f);
		pos.position = make_vector(-1.0f, 0.0f);
	}
	virtual ~BallModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Vector3f > pos;
};

#endif //BALLMODULE_HPP
