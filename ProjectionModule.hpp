#ifndef PROJECTIONMODULE_HPP
#define PROJECTIONMODULE_HPP
#include "Module.hpp"

const float BaseLen = 0.75f;
const float SegLen = 0.5f;
const float Rad = 0.05f;
const Vector2f BasePos = {{{-1.0f + Rad * 1.5f, 0.0f}}};

class ProjectionModule : public Module {
public:
	ProjectionModule() {
		in_ports.push_back(&near);
		in_ports.push_back(&far);
		in_ports.push_back(&fov);
		near() = 1.0f;
		near.name = "near";
		near.position = make_vector(-2.0f, 0.5f);
		far() = 9.0f;
		far.name = "far";
		far.position = make_vector(-2.0f, 0.0f);
		fov() = 60.0f;
		fov.name = "fov";
		fov.position = make_vector(-2.0f, -0.5f);
	}
	virtual ~ProjectionModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< float > near;
	PortDataContainer< float > far;
	PortDataContainer< float > fov;

	vector< Vector2f > points;
};

#endif //PROJECTIONMODULE_HPP
