#ifndef SS_VOLUME_HPP
#define SS_VOLUME_HPP

#include "Module.hpp"

class SoftVolume : public Module {
public:
	SoftVolume();
	virtual ~SoftVolume();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);
};

#endif //SS_VOLUME_HPP
