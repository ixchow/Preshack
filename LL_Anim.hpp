#ifndef LL_ANIM_HPP
#define LL_ANIM_HPP

#include "Module.hpp"

enum {
	BARS,
	BALL,
	EDGES,
	STACKING
};
class LayerAnim : public Module {
public:
	LayerAnim();
	virtual ~LayerAnim();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	int mode;
};

#endif //LL_ANIM_HPP
