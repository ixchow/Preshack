#ifndef DISKMODULE_HPP
#define DISKMODULE_HPP
#include "Module.hpp"

class DiskModule : public Module {
public:
	DiskModule() {
		step = 0;
		animate = false;
	}
	virtual ~DiskModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);
	unsigned int step;
	bool animate;
};

#endif //DISKMODULE_HPP
