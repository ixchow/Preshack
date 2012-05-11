#ifndef TRACEMODULE_HPP
#define TRACEMODULE_HPP
#include "Module.hpp"

#include "GameStuff.hpp"

class TraceModule : public Module {
public:
	TraceModule() {
		in_ports.push_back(&control);
		
		control.position = make_vector(-0.5f * size().x, 0.0f);

		control.name = "control";

		time_acc = 0.0f;

		paused = true;
	}
	virtual ~TraceModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Game2d::Control > control;
	bool paused;
	float time_acc;
};

#endif //TRACEMODULE_HPP
