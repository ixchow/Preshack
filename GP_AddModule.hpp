#ifndef ADDMODULE_HPP
#define ADDMODULE_HPP
#include "Module.hpp"

#include "GP_TexViewerModule.hpp"

class AddModule : public Module {
public:
	AddModule();
	virtual ~AddModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< TexPacket > guess_in;
	PortDataContainer< TexPacket > correction_in;
	PortDataContainer< TexPacket > guess_out;

	bool do_add;
};

#endif //ADDMODULE_HPP
