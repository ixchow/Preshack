#ifndef LL_DIALOG_HPP
#define LL_DIALOG_HPP

#include "Module.hpp"

class LayeringModule;

class LayersDialog : public Module {
public:
	LayersDialog();
	virtual ~LayersDialog();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	Vector2f mouse_pos;
	PortDataContainer< LayeringModule * > layering;
};

#endif //LL_DIALOG_HPP
