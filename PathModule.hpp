#ifndef PATHMODULE_HPP
#define PATHMODULE_HPP
#include "Module.hpp"

#include <map>

class PathModule : public Module {
public:
	PathModule() {
		in_ports.push_back(&leave_a);
		in_ports.push_back(&intermed);
		in_ports.push_back(&join_b);

		out_ports.push_back(&score);

		leave_a.name = "leave";
		intermed.name = "intermed";
		join_b.name = "join";
		score.name = "score";

		leave_a.position = make_vector(-1.5f, 0.5f);
		intermed.position = make_vector(-1.5f,0.0f);
		join_b.position = make_vector(-1.5f,-0.5f);
		score.position = make_vector(1.5f, 0.0f);

		cur_leave_a = (unsigned)-1;
		cur_join_b = (unsigned)-1;
		cur_intermed = (unsigned)-1;

		show_c = false;
		animate = false;
		anim_time = 0.0f;
	}
	virtual ~PathModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);
	//Helper functions:
	void calculate_path();

	PortDataContainer< float > leave_a;
	PortDataContainer< float > intermed;
	PortDataContainer< float > join_b;

	PortDataContainer< float > score;

	vector< Vector2f > path_a;
	vector< Vector2f > path_b;
	vector< Vector2f > path_c; //the 'composite' path.

	unsigned int cur_leave_a;
	unsigned int cur_join_b;
	unsigned int cur_intermed;

	bool show_c;
	bool animate;
	float anim_time;
};

#endif //PATHMODULE_HPP
