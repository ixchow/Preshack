#pragma once

#include "VidStream.hpp"

#include "Module.hpp"
#include <Graphics/Graphics.hpp>
#include <memory>


class VideoModule : public Module {
public:
	VideoModule(std::string const &_path);
	virtual ~VideoModule();

	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	string path;
	std::unique_ptr< VidStream > stream;
	double target_time = 0.0;
	bool paused = true;
	bool dirty = true;
	GLuint tex = 0;
};
