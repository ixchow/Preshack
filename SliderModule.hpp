#ifndef SLIDERMODULE_HPP
#define SLIDERMODULE_HPP
#include "Module.hpp"

class SliderModule : public Module {
public:
	SliderModule() {
		out_ports.push_back(&val);
		val.position = make_vector(1.0f, 0.0f);
		min = -1.0f;
		max = 1.0f;
		snap = 0.0f;
		val() = (min + max) * 0.5f;
		width = 2.0f;
		rate = 0.0f;
		playing = false;
	}
	virtual ~SliderModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< float > val;
	float min;
	float max;
	float snap;
	float width;
	float rate;
	bool playing;
};

#endif //SLIDERMODULE_HPP
