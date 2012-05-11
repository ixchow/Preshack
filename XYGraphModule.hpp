#ifndef XYGRAPHMODULE_HPP
#define XYGRAPHMODULE_HPP
#include "Module.hpp"

#include <map>

class XYGraphModule : public Module {
public:
	XYGraphModule() {
		in_ports.push_back(&x_val);
		in_ports.push_back(&y_val);
		in_ports.push_back(&z_val);
		x_val.name = "x";
		y_val.name = "y";
		z_val.name = "z";
		x_val.position = make_vector( 0.0f,-1.0f);
		y_val.position = make_vector(-1.0f, 0.0f);
		z_val.position = make_vector( 0.0f, 1.0f);
		x_val() = 0.0f;
		y_val() = 0.0f;
		z_val() = 0.0f;
		old_val = make_vector(0.0f, 0.0f, 0.0f);
		min_val = make_vector(-1.0f,-1.0f,-1.0f);
		max_val = make_vector( 1.0f, 1.0f, 1.0f);
		recording = false;
	}
	virtual ~XYGraphModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< float > x_val;
	PortDataContainer< float > y_val;
	PortDataContainer< float > z_val;
	Vector3f old_val;
	Vector3f min_val;
	Vector3f max_val;
	bool recording;
	std::multimap< float, float > pts;
};

#endif //XYGRAPHMODULE_HPP
