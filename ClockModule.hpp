#ifndef CLOCKMODULE_HPP
#define CLOCKMODULE_HPP
#include "Module.hpp"

//for Arm constants...
#include "ArmModule.hpp"

class ClockModule : public Module {
public:
	ClockModule() {
		out_ports.push_back(&accum);
		accum.position = make_vector(0.25f, 0.0f);
		accum() = 0.0f;
		paused = true;
	}
	virtual ~ClockModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< float > accum;
	bool paused;
};

#endif //CLOCKMODULE_HPP
