#ifndef JOYSTICKMODULE_HPP
#define JOYSTICKMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

class JoystickModule : public Module {
public:
	JoystickModule() {
		out_ports.push_back(&control);
		control.position = make_vector(0.5f, 0.0f);
		control.name = "control";
		set = false;
	}
	virtual ~JoystickModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Game2d::Control > control;
	bool set;
};

#endif //JOYSTICKMODULE_HPP
