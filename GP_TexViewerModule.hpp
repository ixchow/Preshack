#ifndef TEXVIEWERMODULE_HPP
#define TEXVIEWERMODULE_HPP
#include "Module.hpp"

#include <Graphics/Graphics.hpp>

class TexPacket {
public:
	TexPacket() : width(0), height(0), tex(0), fb(0), new_flag(false) {
	}
	void dealloc(); //deallocate textures. use with care!
	void alloc(unsigned int width, unsigned int height); //allocate textures -- DOES NOT DEALLOC! (though will warn)
	unsigned int width, height;
	GLuint tex, fb;
	bool new_flag; //some sort of 'hey there, use this texture, eh?' flag.
};

class TexViewerModule : public Module {
public:
	TexViewerModule(bool _container, bool _rotate=false);
	virtual ~TexViewerModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	PortDataContainer< TexPacket > tex;
	PortDataContainer< TexPacket > tex_out;
	bool container;
	bool rotate;
};

#endif //TEXVIEWERMODULE_HPP
