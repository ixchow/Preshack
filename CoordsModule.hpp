#ifndef COORDSMODULE_HPP
#define COORDSMODULE_HPP
#include "Module.hpp"

class CoordsModule : public Module {
public:
	CoordsModule() {
		in_ports.push_back(&time);
		in_ports.push_back(&coords);
		time.position = make_vector(-1.0f, 0.25f);
		coords.position = make_vector(-1.0f,-0.25f);
		current() = make_vector(0.0f, 0.0f, 0.0f);
		time() = 0.0f;
		out_ports.push_back(&current);
		current.position = make_vector(1.0f, 0.0f);
	}
	virtual ~CoordsModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	//input:
	PortDataContainer< vector< Vector3f > > coords;
	PortDataContainer< float > time;
	//output:
	PortDataContainer< Vector3f > current;
};

#endif //COORDSMODULE_HPP
