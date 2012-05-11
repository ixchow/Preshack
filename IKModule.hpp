#ifndef IKMODULE_HPP
#define IKMODULE_HPP
#include "Module.hpp"

//for Arm constants...
#include "ArmModule.hpp"

class IKModule : public Module {
public:
	IKModule() {
		in_ports.push_back(&hand);
		hand() = make_vector(BaseLen+SegLen+BasePos.x, BasePos.y, 0.0f);
		hand.position = make_vector(-0.25f, 0.0f);

		out_ports.push_back(&base_ang);
		out_ports.push_back(&seg_ang);
		out_ports.push_back(&hand_ang);
		base_ang() = seg_ang() = hand_ang() = 0.0f;
		base_ang.position = make_vector(0.25f, 1.0f / 6.0f - 0.5f);
		seg_ang.position  = make_vector(0.25f, 3.0f / 6.0f - 0.5f);
		hand_ang.position = make_vector(0.25f, 5.0f / 6.0f - 0.5f);
	}
	virtual ~IKModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< Vector3f > hand;

	PortDataContainer< float > base_ang;
	PortDataContainer< float > seg_ang;
	PortDataContainer< float > hand_ang;

};

#endif //IKMODULE_HPP
