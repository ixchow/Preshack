#ifndef ANIMATIONMODULE_HPP
#define ANIMATIONMODULE_HPP
#include "Module.hpp"

#include <Graphics/Texture.hpp>

class AnimationModule : public Module {
public:
	AnimationModule(std::string params);
	virtual ~AnimationModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual bool get_pixel_size(Vector2ui &into);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	vector< std::string > filenames;
	unsigned int frame;
	std::string bad_filename;
	bool no_frame;
};

#endif //ANIMATIONMODULE_HPP
