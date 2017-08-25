#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#ifdef WINDOWS
#define _WINUSER_
#endif

#include <SDL.h>
#define NO_SDL_GLEXT
#define __glext_h_
#include <SDL_opengl.h>
#undef __glext_h_
#include <Graphics/glext.h>
#include <string>


#ifdef MACOSX
#define glActiveTextureARB glActiveTexture
#define glMultiTexCoord2fARB glMultiTexCoord2f
#define glMultiTexCoord2iARB glMultiTexCoord2i
#endif

using std::string;

namespace Graphics {
	//these can be set up before calling init()
	extern int screen_x; //default 800
	extern int screen_y; //default 600
	extern bool fullscreen; //default false
	extern SDL_Window *window;
	extern SDL_GLContext context;

	//convenience function for screen aspect ratio.
	inline double aspect() {
		return (double)screen_x / (double)screen_y;
	}
	inline float aspectf() {
		return (float)screen_x / (float)screen_y;
	}

	enum {
		NEED_ACCUM = 1 << 0,
		NEED_STENCIL = 1 << 1,
		WANT_VSYNC = 1 << 2,
		WANT_MULTISAMPLE4 = 1 << 3,
		WANT_MULTISAMPLE8 = 1 << 4,
		WANT_MULTISAMPLE16 = 1 << 5,
		WANT_EXACT_SIZE = 1 << 6, //otherwise, Graphics::screen_x and Graphics::screen_y might change, esp. in fullscreen.
	};
	//set up graphics output. lets you know if it fails.
	bool init(int flags = 0);

	//tear down graphics output.
	void deinit();

	//check if GL is initialized.
	bool initialized();

	//register a function to be called next init.
	//call only if !is_initialized().
	void on_init(void (*func)(void *), void *data);

	//helper function for gl code.
	//prints out all the current gl errors along with the 'where' string.
	void gl_errors(string const &where);

	//probably some stuff for fullscreen callback registration here...
	//probably some opengl texture stuff here also...
	//probably some stuff for text drawing...
	//maybe some stuff for extension loading -- use GLExtensions.hpp
}


#endif
