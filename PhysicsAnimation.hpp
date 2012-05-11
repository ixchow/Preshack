#ifndef PHYSICSANIMATION_HPP
#define PHYSICSANIMATION_HPP

#include "Module.hpp"

#include <Graphics/Texture.hpp>

#include <vector>

class PhysicsAnimationModule : public Module {
public:
	PhysicsAnimationModule(std::string params);
	virtual ~PhysicsAnimationModule();

	void add_frame(std::string filename);

	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	std::vector< Graphics::TextureRef > textures;
	std::vector< Vector2f > coms;

	float t;
	float dt;

	Vector2f up;

	bool dragging;
	Vector2f dragging_ref;

	bool draw_coms;
};

#endif //PHYSICSANIMATION_HPP
