#ifndef RHSMODULE_HPP
#define RHSMODULE_HPP
#include "Module.hpp"

#include "GP_TexViewerModule.hpp"

class RHSModule : public Module {
public:
	RHSModule();
	virtual ~RHSModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< TexPacket > gx_in;
	PortDataContainer< TexPacket > gy_in;
	PortDataContainer< TexPacket > rhs_out;
};

#endif //RHSMODULE_HPP
