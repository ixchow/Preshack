#ifndef LEVELMODULE_HPP
#define LEVELMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

#include <Character/Character.hpp>

#include <utility>
using std::pair;
using std::make_pair;

class LevelModule : public Module {
public:
	LevelModule() {
		frame.name = "frame";
		status.name = "status";
		in_ports.push_back(&frame);
		in_ports.push_back(&status);
	
		frame.position = make_vector(-this->size().x * 0.5f, this->size().y * ( 0.25f));
		status.position = make_vector(-this->size().x * 0.5f, this->size().y * (-0.25f));
		status().pos = make_vector(0.5f, 0.0f);
		status().flip = false;
		frame().motion = (unsigned)-1;
		frame().frame = (unsigned)-1;
		current_pose.clear();
	}
	virtual ~LevelModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

private:
	PortDataContainer< Game2d::LevelStatus > status;
	PortDataContainer< Game2d::Frame > frame;
	Character::Pose current_pose;
};

#endif //LEVELMODULE_HPP
