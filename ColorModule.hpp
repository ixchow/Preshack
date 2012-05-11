#ifndef COLORMODULE_HPP
#define COLORMODULE_HPP
#include "Module.hpp"

class ColorModule : public Module {
public:
	ColorModule() {
		set_color(make_vector(0.5f, 0.5f, 0.5f));
		out_ports.push_back(&color);
		color.name = "color";
		color.position = make_vector(1.0f, 0.0f);
		dragging_triangle = false;
	}
	virtual ~ColorModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	void set_color(Vector3f rgb_color);
	bool dragging_triangle;
	void handle_position(Vector2f pos); //helper for handle_event.
	PortDataContainer< Vector3f > color;
	Vector3f hsv_color;
};

#endif //COLORMODULE_HPP
