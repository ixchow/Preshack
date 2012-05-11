#ifndef MOTIONMODULE_HPP
#define MOTIONMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

class MotionModule : public Module {
public:
	MotionModule(unsigned int _index) : index(_index) {
		time_acc = 0.0f;
		init();
	}
	virtual ~MotionModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	unsigned int index;
	float time_acc;

private:
	void init(); //precalc some draw data for motions.
	float max_pos,min_pos;
	bool paused;
	vector< float > frame_positions;
};

#endif //MOTIONMODULE_HPP
