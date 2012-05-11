#ifndef RESIDUALMODULE_HPP
#define RESIDUALMODULE_HPP
#include "Module.hpp"

#include "GP_TexViewerModule.hpp"

class ResidualModule : public Module {
public:
	ResidualModule();
	virtual ~ResidualModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< TexPacket > guess_in;
	PortDataContainer< TexPacket > rhs_in;
	PortDataContainer< TexPacket > rhs_out;

	float corner, edge, middle;

	bool do_residual;
};

#endif //RESIDUALMODULE_HPP
