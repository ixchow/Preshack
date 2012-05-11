#ifndef CONTROLLERMODULE_HPP
#define CONTROLLERMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

class ControllerModule : public Module {
public:
	ControllerModule() {
		in_ports.push_back(&control);
		out_ports.push_back(&frame);
		
		control.position = make_vector(-0.25f, 0.0f);
		frame.position = make_vector( 0.25f, 0.0f);

		fragment = 0;
		time_acc = 0;

		frame().motion = (unsigned)-1;
	}
	virtual ~ControllerModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

private:

	PortDataContainer< Game2d::Control > control;

	unsigned int fragment;
	float time_acc;
	PortDataContainer< Game2d::Frame > frame;
};

#endif //CONTROLLERMODULE_HPP
