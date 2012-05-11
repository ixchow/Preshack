#ifndef RESTRICTMODULE_HPP
#define RESTRICTMODULE_HPP
#include "Module.hpp"

#include "GP_TexViewerModule.hpp"

class RestrictModule : public Module {
public:
	RestrictModule();
	virtual ~RestrictModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< TexPacket > in;
	PortDataContainer< TexPacket > out;

	bool do_restrict;
};

#endif //RESTRICTMODULE_HPP
