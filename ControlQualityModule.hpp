#ifndef CONTROLQUALITYMODULE_HPP
#define CONTROLQUALITYMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

class ControlQualityModule : public Module {
public:
	ControlQualityModule() {
		in_ports.push_back(&frag);
		in_ports.push_back(&bin);

		frag.position = make_vector(-0.5f * size().x, 0.25f * size().y);
		bin.position = make_vector(-0.5f * size().x,-0.25f * size().y);

		frag() = (unsigned)-1;
		bin() = (unsigned)-1;

		frag.name = "frag";
		bin.name = "bin";

		out_ports.push_back(&value);

		value.position = make_vector(0.5f * size().x, 0.0f);

		value() = 0.0f;

		value.name = "value";

	}
	virtual ~ControlQualityModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< unsigned int > frag;
	PortDataContainer< unsigned int > bin;
	PortDataContainer< float > value;
};

#endif //CONTROLQUALITYMODULE_HPP
