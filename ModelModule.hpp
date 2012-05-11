#ifndef MODELMODULE_HPP
#define MODELMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

class ModelModule : public Module {
public:
	ModelModule() {
		in_ports.push_back(&index1);
		in_ports.push_back(&index2);

		index1.position = make_vector(-0.5f, 0.25f);
		index2.position = make_vector(-0.5f,-0.25f);

		index1() = (unsigned)-1;
		index2() = (unsigned)-1;

		index1.name = "index1";
		index2.name = "index2";

		out_ports.push_back(&value);

		value.position = make_vector(0.5f, 0.0f);

		value() = 0.0f;

		value.name = "value";

	}
	virtual ~ModelModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< unsigned int > index1;
	PortDataContainer< unsigned int > index2;
	PortDataContainer< float > value;
};

#endif //MODELMODULE_HPP
