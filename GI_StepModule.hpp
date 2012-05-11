#ifndef STEPMODULE_HPP
#define STEPMODULE_HPP
#include "Module.hpp"

#include <Graphics/Graphics.hpp>

class TexInfo {
public:
	TexInfo() : size(make_vector(0U, 0U)), tex(0), seq(0) {
	}
	void dealloc(); //deallocate textures. use with care!
	void alloc(Vector2ui _size);
	Vector2ui size;
	GLuint tex;
	unsigned int seq; //when updated, seq gets +1.
};

class StepModule : public Module {
public:
	StepModule();
	virtual ~StepModule();
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	GLuint fb;

	PortDataContainer< TexInfo > tex;
	PortDataContainer< TexInfo > high_tex;
	PortDataContainer< TexInfo > low_tex;
	PortDataContainer< TexInfo > over_tex;
};

#endif //STEPMODULE_HPP
