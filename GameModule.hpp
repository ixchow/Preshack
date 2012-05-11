#ifndef GAMEMODULE_HPP
#define GAMEMODULE_HPP

#include "Module.hpp"

#include "GameStuff.hpp"

#include <utility>
using std::pair;
using std::make_pair;

class GameModule : public Module {
public:
	GameModule() {
		in_control.name = "cont";
		out_control.name = "cont";
		status.name = "status";
		in_ports.push_back(&in_control);
		out_ports.push_back(&out_control);
		out_ports.push_back(&status);
		
		in_control.position = make_vector(-0.25f, 0.0f);

		out_control.position = make_vector( 0.25f, 3.0f / 4.0f -0.5f);
		status.position = make_vector( 0.25f, 1.0f / 4.0f -0.5f);

		flip_timer = 0.0f;

		spawn();
	}
	virtual ~GameModule() {
	}

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

private:
	void spawn(); //spawn player

	PortDataContainer< Game2d::Control > in_control;
	PortDataContainer< Game2d::Control > out_control;

	float flip_timer;

	PortDataContainer< Game2d::LevelStatus > status;
	PortDataContainer< Vector2f > vel; //not connected, but consistency.
};

#endif //GAMEMODULE_HPP
