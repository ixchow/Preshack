#include "Screen.hpp"


#include <Graphics/Graphics.hpp>

#include <Basis/Error.hpp>

#include <vector>
#include <deque>

#include <cassert>

using std::vector;

#ifdef BASIS_USE_NAMESPACE
namespace Basis {
#endif //BASIS_USE_NAMESPACE

Screen::Screen() {
	//occupies the entire screen:
	pixel_extents.max = make_vector< float >(Graphics::screen_x, Graphics::screen_y);
	pixel_extents.min = make_vector< float >(0,0);
	extents.max = make_vector(Graphics::aspectf(), 1.0f);
	extents.min =-make_vector(Graphics::aspectf(), 1.0f);
}

float Screen::aspectf() const {
	return extents.size().x / extents.size().y;
}

double Screen::aspect() const {
	return extents.size().x / extents.size().y;
}

void Screen::update_screen_mouse_relative(SDL_MouseMotionEvent const &motion, Vector2f &mouse) {
	mouse.x += motion.xrel / pixel_extents.size().x * 2.0f * aspectf();
	mouse.y -= motion.yrel / pixel_extents.size().y * 2.0f;
}

void Screen::update_screen_mouse_absolute(SDL_MouseMotionEvent const &motion, Vector2f &mouse) {
	mouse.x =  (motion.x - pixel_extents.min.x) / (pixel_extents.max.x - pixel_extents.min.x) * 2.0f * aspectf() - aspectf();
	mouse.y =  (motion.y - pixel_extents.min.y) / (pixel_extents.max.y - pixel_extents.min.y) *-2.0f + 1.0f;
}

void Screen::update_screen_mouse_relative(SDL_MouseMotionEvent const &motion, Vector2d &mouse) {
	mouse.x += motion.xrel / pixel_extents.size().x * 2.0 * aspect();
	mouse.y -= motion.yrel / pixel_extents.size().y * 2.0;
}

void Screen::update_screen_mouse_absolute(SDL_MouseMotionEvent const &motion, Vector2d &mouse) {
	mouse.x =  (motion.x - pixel_extents.min.x) / (pixel_extents.max.x - pixel_extents.min.x) * 2.0 * aspect() - aspect();
	mouse.y =  (motion.y - pixel_extents.min.y) / (pixel_extents.max.y - pixel_extents.min.y) *-2.0 + 1.0;
}


namespace {
	class Layer {
	public:
		Layer(Screen *_screen) : screen(_screen), quit_flag(false) { }
		Screen *screen;
		bool quit_flag;
	};
	vector< Layer > layers;
};

namespace {
	class RunProc {
	public:
		RunProc(void (*_fn)(void *), void *_data) : fn(_fn), data(_data), remove(false) {
		}
		void operator()() {
			fn(data);
		}
		void (*fn)(void *);
		void *data;

		bool remove;

	};
	vector< RunProc > &get_procs() {
		static vector< RunProc > procs;
		return procs;
	}
	class EventProc {
	public:
		EventProc(bool (*_fn)(SDL_Event const &, void *), void *_data) : fn(_fn), data(_data), remove(false) {
		}
		bool operator()(SDL_Event const &e) {
			return fn(e, data);
		}
		bool (*fn)(SDL_Event const &, void *);
		void *data;

		bool remove;

	};
	vector< EventProc > &get_eventprocs() {
		static vector< EventProc > procs;
		return procs;
	}

};

Uint32 min_ms_per_frame = 1000/100;
Uint32 max_ms_per_frame = 1000/15;
double avg_ms_per_frame = 0.0;

namespace {
	void trim_layers(size_t min) {
		while ((layers.size() > min) && layers.back().quit_flag) {
			layers.pop_back();
		}
	}
};

static void run(bool *forced_to_quit) {
	if (forced_to_quit) {
		*forced_to_quit = false;
	}
	Uint32 env_t = SDL_GetTicks();
	Uint32 env_et = 0;
	SDL_Event event;

	//Don't trim past the last screen we're running.
	// -- screens have *weird* runloop semantics.
	size_t min = layers.size();
	assert(min > 0);
	min -= 1;

	trim_layers(min);
	while (layers.size() > min) {

		{ //sweep through runprocs and run 'em:
			vector< RunProc > &procs = get_procs();
			for (vector< RunProc >::iterator i = procs.begin(); i != procs.end(); /* later */ ) {
				if (i->remove) {
					i = procs.erase(i);
				} else {
					(*i)();
					++i;
				}
			}
		}

		trim_layers(min);
		if (layers.size() <= min) break;

		//Find elapsed milliseconds, delaying and clamping as needed.
		env_et = SDL_GetTicks() - env_t;
		while (env_et < min_ms_per_frame) {
			SDL_Delay(min_ms_per_frame - env_et);
			env_et = SDL_GetTicks() - env_t;
		}
		env_t += env_et;

		if (env_et > max_ms_per_frame) env_et = max_ms_per_frame;

		Uint32 before = SDL_GetTicks();

		//get and process any new events.
		while (SDL_PollEvent(&event) && layers.size() > min) {
			bool handled = false;
			{
				vector< EventProc > &procs = get_eventprocs();
				for (vector< EventProc >::iterator p = procs.begin(); p != procs.end(); ++p) {
					if ( (*p)(event) ) {
						handled = true;
						break;
					}
				}
			}
			assert(layers.back().screen);
			if (!handled) {
				layers.back().screen->handle_event(event);
			}
			//add alt-escape as a sort of force-quit:
			if ((event.type == SDL_KEYDOWN
				&& event.key.keysym.sym == SDLK_ESCAPE
#ifdef WINDOWS
				&& (event.key.keysym.mod & KMOD_SHIFT))
#else
				&& (event.key.keysym.mod & KMOD_ALT))
#endif
			 || event.type == SDL_QUIT) {
				//everything must quit:
				for (unsigned int l = 0; l < layers.size(); ++l) {
					layers[l].quit_flag = true;
				}
				if (forced_to_quit) {
					*forced_to_quit = true;
				}
			}
			trim_layers(min);
		}

		if (layers.size() <= min) break;
	
		//update the world
		assert(layers.back().screen);
		layers.back().screen->update(env_et / 1000.0f);

		trim_layers(min);
		if (layers.size() <= min) break;
	
		//draw the world
		assert(layers.back().screen);
		layers.back().screen->draw();

		//swap the drawn world to the front buffer
		SDL_GL_SwapWindow(Graphics::window);
		Uint32 after = SDL_GetTicks();

		//Processing time, including rendering and maybe vsync time
		static std::deque< Uint32 > frame_times;
		frame_times.push_back(after - before);
		if (frame_times.size() > 10) {
			frame_times.pop_front();
		}
		avg_ms_per_frame = 0.0;
		for (auto t = frame_times.begin(); t != frame_times.end(); ++t) {
			avg_ms_per_frame += *t;
		}
		if (frame_times.size() > 0) {
			avg_ms_per_frame /= frame_times.size();
		}

		trim_layers(min);
		if (layers.size() <= min) break;
	}
}

void start_screen(Screen *screen, bool *forced_to_quit) {
	layers.push_back(Layer(screen));
	run(forced_to_quit);
}

void quit_screen(Screen *me) {
	unsigned int l = layers.size() - 1;
	while (l < layers.size() && layers[l].quit_flag) --l;
	if (l >= layers.size()) {
		LOG_ERROR( "trying to quit, and all layers already quit." );
		return;
	}
	if (me != layers[l].screen) {
		LOG_ERROR( "trying to quit the wrong screen." );
		return;
	}
	layers[l].quit_flag = true;
}

Screen *prev_screen( Screen *me ) {
	for (unsigned int l = layers.size() - 1; l < layers.size(); --l) {
		if (layers[l].screen == me) {
			if (l > 0) return layers[l-1].screen;
			else return NULL;
		}
	}
	return NULL;
}

Screen *next_screen( Screen *me ) {
	for (unsigned int l = 0; l < layers.size(); ++l) {
		if (layers[l].screen == me) {
			if (l + 1 < layers.size()) return layers[l+1].screen;
			else return NULL;
		}
	}
	return NULL;
}

void add_runproc( void (*fn)(void *), void *data ) {
	vector< RunProc > &procs = get_procs();
	procs.push_back(RunProc(fn, data));
}

void remove_runproc( void (*fn)(void *), void *data) {
	vector< RunProc > &procs = get_procs();
	for (vector< RunProc >::iterator i = procs.begin(); i != procs.end(); ++i) {
		if (i->fn == fn && i->data == data) {
			procs.erase(i);
			break;
		}
	}
}

void add_eventproc( bool (*fn)(SDL_Event const &, void *), void *data ) {
	vector< EventProc > &procs = get_eventprocs();
	procs.push_back(EventProc(fn, data));
}

void remove_eventproc( bool (*fn)(SDL_Event const &, void *), void *data) {
	vector< EventProc > &procs = get_eventprocs();
	for (vector< EventProc >::iterator i = procs.begin(); i != procs.end(); ++i) {
		if (i->fn == fn && i->data == data) {
			procs.erase(i);
			break;
		}
	}
}


#ifdef BASIS_USE_NAMESPACE
} //namespace Basis
#endif //BASIS_USE_NAMESPACE
