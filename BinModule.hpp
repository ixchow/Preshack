#ifndef BINMODULE_HPP
#define BINMODULE_HPP
#include "Module.hpp"

#include "GameStuff.hpp"

class BinModule : public Module {
public:
	BinModule() {
		in_ports.push_back(&control);
		out_ports.push_back(&index);
		
		control.position = make_vector(-0.5f * size().x, 0.0f);
		index.position = make_vector( 0.5f * size().x, 0.0f);

		control.name = "control";
		index.name = "index";

		index() = 0;
	}
	virtual ~BinModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Game2d::Control > control;
	PortDataContainer< unsigned int > index;
};

#endif //BINMODULE_HPP
