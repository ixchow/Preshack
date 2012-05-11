#ifndef VIDEOMODULE_HPP
#define VIDEOMODULE_HPP
#include "VidStream.hpp"

#include "Module.hpp"
#include <Graphics/Graphics.hpp>


class VideoModule : public Module {
public:
	VideoModule(std::string const &_path) : path(_path) {
		paused = true;
		if (path.size() > 4 && path.substr(path.size()-4) == "auto") {
			paused = false;
			path.erase(path.size()-4,4);
		}
		if (!stream.open(path)) {
			cerr << "WARNING: video module can't open '" << path << "'." << endl;
		} else if (!stream.has_video()) {
			cerr << "WARNING: stream '" << path << "' doesn't contain video." << endl;
		}
		target_time = 0.0f;
		tex = 0;
		glGenTextures(1, &tex);
		dirty = true;
		for (unsigned int i = 0; i < 10; ++i) {
			if (!stream.advance_video()) break;
			stream.discard_audio();
		}
	}
	virtual ~VideoModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	string path;
	VidStream stream;
	double target_time;
	bool paused;
	bool dirty;
	GLuint tex;

};

#endif //VIDEOMODULE_HPP
