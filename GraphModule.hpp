#ifndef GRAPHMODULE_HPP
#define GRAPHMODULE_HPP
#include "Module.hpp"

class GraphModule : public Module {
public:
	GraphModule() {
		in_ports.push_back(&time);
		in_ports.push_back(&graph);
		time.position = make_vector(-1.0f, 0.25f);
		time.name = "time";
		graph.position = make_vector(-1.0f,-0.25f);
		graph.name = "graph";
		current() = 0.0f;
		time() = 0.0f;
		out_ports.push_back(&current);
		current.position = make_vector(1.0f, 0.0f);
		current.name = "val";
	}
	virtual ~GraphModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	//input:
	PortDataContainer< vector< float > > graph;
	PortDataContainer< float > time;
	//output:
	PortDataContainer< float > current;
};

#endif //GRAPHMODULE_HPP
