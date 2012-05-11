#ifndef CONFORMALMODULE_HPP
#define CONFORMALMODULE_HPP
#include "Module.hpp"
#include <Graphics/Graphics.hpp>

class ConformalModule : public Module {
public:
	ConformalModule() {
		in_ports.push_back(&src_steps);
		in_ports.push_back(&src_scale);
		in_ports.push_back(&rot_mult);
		in_ports.push_back(&scale_mult);
		src_steps() = 0.0f;
		src_steps.name = "src_steps";
		src_steps.position = make_vector(-1.0f, 0.75f);
		src_scale() = 2.0f;
		src_scale.name = "src_scale";
		src_scale.position = make_vector(-1.0f, 0.25f);
		rot_mult() = 1.0f;
		rot_mult.name = "2pi_mult";
		rot_mult.position = make_vector(-1.0f,-0.25f);
		scale_mult() = 0.0f;
		scale_mult.name = "scale_mult";
		scale_mult.position = make_vector(-1.0f,-0.75f);
		image = "infty/grid.png";
		tween = 0.0f;
		animate = false;
		nomap = false;
		lattice = false;
		exp_tex = 0;
		exp_tex_size = 0.0f;
	}
	virtual ~ConformalModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	string image;
	PortDataContainer< float > rot_mult;
	PortDataContainer< float > scale_mult;
	PortDataContainer< float > src_steps;
	PortDataContainer< float > src_scale;
	float tween;
	bool animate;
	bool nomap;
	bool lattice;
	GLuint exp_tex;
	float exp_tex_size;
	GLuint log_tex;
	Vector2f log_tex_fac;
};

#endif //CONFORMALMODULE_HPP
