#ifndef MANYDGRAPHMODULE_HPP
#define MANYDGRAPHMODULE_HPP
#include "Module.hpp"

#include <Graphics/Texture.hpp>

class ManyDGraphModule : public Module {
public:
	ManyDGraphModule(std::string params);
	virtual ~ManyDGraphModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//convenience:
	void update_current_axis(Vector2f mouse);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	unsigned int in_size;
	vector< float > in_data;
	vector< Vector3f > out_data;

	vector< Vector4f > axis;
	vector< Vector4f > goal_axis;
	float goal_time;
	unsigned int current_axis;
};

#endif //MANYDGRAPHMODULE_HPP
