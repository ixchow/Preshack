#include "Graphics.hpp"
#ifdef OSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <iostream>
#include <list>
#include <utility>
#include <assert.h>

using std::cerr;
using std::endl;

using std::list;
using std::pair;
using std::make_pair;

namespace Graphics {

int screen_x = 800;
int screen_y = 600;
bool fullscreen = false;
SDL_Window *window = NULL;
SDL_GLContext context = NULL;

namespace { //private-type variables.
bool sub_inited = false;
bool inited = false;

list< pair< void (*)(void *), void * > > & get_init_funcs() {
	static list< pair< void (*)(void *), void * > > init_funcs;
	return init_funcs;
}
} //end anon namespace

bool init(int flags) {
	
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == 0) {
		sub_inited = true;
		
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8);
		if (flags & NEED_ACCUM) {
			SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 16);
			SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 16);
			SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 16);
		}
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0);
		//SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 32);
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24);
		if (flags & NEED_STENCIL) {
			SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8);
		}
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1);
		if (flags & WANT_MULTISAMPLE16) {
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 16 );
		} else if (flags & WANT_MULTISAMPLE8) {
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 8 );
		} else if (flags & WANT_MULTISAMPLE4) {
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );
		} else {
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );
		}

		do {
			if (fullscreen) {
				window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_x, screen_y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
			} else {
				window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_x, screen_y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
			}
			if (!window) {
				cerr << "Could not create video surface: " << SDL_GetError() << endl;
				break;
			}
			int w = 0, h = 0;
			SDL_GetWindowSize(window, &w, &h);
			if (screen_x != w || screen_y != h) {
				cerr << "Asked for a screen of size " << screen_x << "x" << screen_y << "; got one of size " << w << "x" << h << "." << endl;
				if (flags & WANT_EXACT_SIZE) {
					break;
				} else {
					screen_x = w;
					screen_y = h;
					SDL_DestroyWindow(window);
					window = NULL;
				}
			}
		} while (!window);

		if (window) {
			context = SDL_GL_CreateContext(window);
			if (!context) {
				cerr << "Could not create opengl context: " << SDL_GetError() << endl;
			} else {
				if (flags & WANT_VSYNC) {
					if (SDL_GL_SetSwapInterval(1) != 0) {
						cerr << "Error setting up VSYNC " << SDL_GetError() << "; will continue, but there may be tearing." << endl;
					}
				}
				inited = true;
			}
		}
	} else {
		cerr << "Could not init video: " << SDL_GetError() << endl;
	}

	if (sub_inited && !inited) {
		deinit();
	}

	if (inited) {
		while (!get_init_funcs().empty()) {
			get_init_funcs().back().first(get_init_funcs().back().second);
			get_init_funcs().pop_back();
		}
	}
	
	return inited;

}

void deinit() {
	if (sub_inited) {
		SDL_GL_DeleteContext(context);
		context = NULL;
		SDL_DestroyWindow(window);
		window = NULL;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		sub_inited = false;
		inited = false;
	}
}

bool initialized() {
	return inited;
}

void on_init(void (*func)(void *), void *data) {
	assert(!initialized()); //only to be called before init.
	get_init_funcs().push_back(make_pair(func, data));
}

void gl_errors(string const &where) {
	GLuint err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		cerr << "(in " << where << ") OpenGL error #" << err
		     << ": ";
		//<< gluErrorString(err) << endl;
		switch (err) {
		#define ERRCASE( X ) \
			case GL_ ## X: \
				cerr << #X; \
				break;
			ERRCASE( INVALID_ENUM )
			ERRCASE( INVALID_VALUE )
			ERRCASE( INVALID_OPERATION )
			ERRCASE( INVALID_FRAMEBUFFER_OPERATION )
			ERRCASE( OUT_OF_MEMORY )
			ERRCASE( STACK_UNDERFLOW )
			ERRCASE( STACK_OVERFLOW )
			default:
				cerr << "(unknown)";
		#undef ERRCASE
		}
		cerr << endl;
	}
}

}
