#ifndef CURVEMODULE_HPP
#define CURVEMODULE_HPP
#include "Module.hpp"

class CurveModule : public Module {
public:
	CurveModule() {
		in_ports.push_back(&points);
		points.position = make_vector(-1.0f, 0.0f);
		out_ports.push_back(&coords);
		coords.position = make_vector(1.0f, 0.0f);
		current = 0;
		twist = false;
	}
	virtual ~CurveModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	void recalc();

	//input:
	PortDataContainer< vector< Vector4f > > points;
	//curve points -> x, y, rot, time.
	//output:
	PortDataContainer< vector< Vector3f > > coords;

	unsigned int current;
	bool twist;
};

#endif //CURVEMODULE_HPP
