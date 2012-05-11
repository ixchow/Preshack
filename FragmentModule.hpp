#ifndef FRAGMENTMODULE_HPP
#define FRAGMENTMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

//for communication with the MotionModule
extern unsigned int CurrentFragment;

class FragmentModule : public Module {
public:
	FragmentModule(unsigned int _index) {
		index() = _index;
		assert(index() < Game2d::fragments.size());
		paused = true;
		time_acc = 0.0f;
		out_ports.push_back(&index);
		index.position = make_vector(0.5f * this->size().x, 0.0f);
		index.name = "index";
		in_ports.push_back(&in_index);
		in_index.position = make_vector(-0.5f * this->size().x, 0.0f);
		in_index.name = "in_index";
		in_index() = (unsigned)-1;
	}
	virtual ~FragmentModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	bool paused;
	float time_acc;
	PortDataContainer< unsigned int > index;
	PortDataContainer< unsigned int > in_index;
};

#endif //FRAGMENTMODULE_HPP
