#ifndef INPUTMODULE_HPP
#define INPUTMODULE_HPP
#include "Module.hpp"

template< class DATA >
class InputModule : public Module {
public:
	InputModule(std::string const &_prefix) : prefix(_prefix) {
		out_ports.push_back(&data);
		data.position = make_vector(0.5f * size().x, 0.0f);
		data.name = "data";
	}
	virtual ~InputModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< DATA > data;
	std::string prefix;
};

#endif //INPUTMODULE_HPP
