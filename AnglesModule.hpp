#ifndef ANGLESMODULE_HPP
#define ANGLESMODULE_HPP
#include "Module.hpp"

//for Arm constants...
#include "ArmModule.hpp"

class AnglesModule : public Module {
public:
	AnglesModule() {
		in_ports.push_back(&base_ang);
		in_ports.push_back(&seg_ang);
		in_ports.push_back(&hand_ang);
		base_ang() = seg_ang() = hand_ang() = 0.0f;
		base_ang.position = make_vector(-0.25f, 1.0f / 6.0f - 0.5f);
		seg_ang.position  = make_vector(-0.25f, 3.0f / 6.0f - 0.5f);
		hand_ang.position = make_vector(-0.25f, 5.0f / 6.0f - 0.5f);


		out_ports.push_back(&base);
		out_ports.push_back(&seg);
		out_ports.push_back(&hand);
		base() = make_vector(BasePos.x, BasePos.y, 0.0f);
		seg() = make_vector(BaseLen+BasePos.x, BasePos.y, 0.0f);
		hand() = make_vector(BaseLen+SegLen+BasePos.x, BasePos.y, 0.0f);
		base.position = make_vector(0.25f, 1.0f / 6.0f - 0.5f);
		seg.position  = make_vector(0.25f, 3.0f / 6.0f - 0.5f);
		hand.position = make_vector(0.25f, 5.0f / 6.0f - 0.5f);
	}
	virtual ~AnglesModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< float > base_ang;
	PortDataContainer< float > seg_ang;
	PortDataContainer< float > hand_ang;

	PortDataContainer< Vector3f > base;
	PortDataContainer< Vector3f > seg;
	PortDataContainer< Vector3f > hand;
};

#endif //ANGLESMODULE_HPP
