#ifndef LL_CODE_HPP
#define LL_CODE_HPP

#include "Module.hpp"

#include <Graphics/Texture.hpp>

class FlipState;

class Code : public Module {
public:
	Code();
	virtual ~Code();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	Graphics::TextureRef code;

	PortDataContainer< FlipState * > state;
};

#endif //LL_CODE_HPP
