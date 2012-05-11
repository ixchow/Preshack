#ifndef PHYSICSMODULE_HPP
#define PHYSICSMODULE_HPP
#include "Module.hpp"

class PhysicsModule : public Module {
public:
	PhysicsModule() {
		in_ports.push_back(&pos);
		in_ports.push_back(&vel);
		
		pos.position = make_vector(-0.25f, 1.0f / 4.0f - 0.5f);
		vel.position  = make_vector(-0.25f, 3.0f / 4.0f - 0.5f);

		out_ports.push_back(&coords);
		coords.position = make_vector(0.25f, 0.0f);
	}
	virtual ~PhysicsModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Vector3f > pos;
	PortDataContainer< Vector3f > vel;

	PortDataContainer< vector< Vector3f > > coords;
};

#endif //PHYSICSMODULE_HPP
