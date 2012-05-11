
#include "PresHack.hpp"
#include "HQ.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>

#include <SDL.h>
#include <SDL_opengl.h>

#include <iostream>
#include <fstream>
#include <cassert>

using std::cout;
using std::endl;
using std::ifstream;

using std::make_pair;

REQUEST_GL_EXTENSION(GL_EXT_framebuffer_object);
REQUEST_GL_EXTENSION(GL_EXT_packed_depth_stencil);
REQUEST_GL_EXTENSION(GL_ARB_texture_non_power_of_two);
REQUEST_GL_EXTENSION(GL_ARB_texture_float);
REQUEST_GL_EXTENSION(GL_ARB_texture_rectangle);

int main(int argc, char **argv) {

	if (SDL_Init(SDL_INIT_TIMER) != 0) {
		cout << "Could not initialize sdl: " << SDL_GetError() << endl;
		return 1;
	}

	//It needs to be big to test line widths properly.
	Graphics::screen_x = 1024;
	Graphics::screen_y = 768;

	bool do_hq = false;
	unsigned int hq_samples = 0;

	for (int arg = 2; arg < argc; ++arg) {
		string args = argv[arg];
		if (args == "--fullscreen") {
			Graphics::fullscreen = true;
		} else if (args == "--lapscreen") {
			Graphics::fullscreen = true;
			const unsigned int Size = 800;
			Graphics::screen_y = Size;
			Graphics::screen_x = Size*1024/768;
		} else if (args == "--smallish") {
			const unsigned int Size = 710;
			Graphics::screen_y = Size;
			Graphics::screen_x = Size*1024/768;
		} else if (args == "--box") {
			Graphics::screen_x = 1024;
			Graphics::screen_y = 1024 * 9 / 16;
		} else if (args == "--720") {
			Graphics::screen_x = 1280;
			Graphics::screen_y = 720;
		} else if (args == "--800") {
			Graphics::screen_x = 1280;
			Graphics::screen_y = 800;
		} else if (args == "--1080") {
			Graphics::screen_x = 1920;
			Graphics::screen_y = 1080;
		} else if (args == "--largeish") {
			const unsigned int Size = 1150;
			Graphics::screen_y = Size;
			Graphics::screen_x = Size*1024/768;
		} else if (args == "--hqsamples") {
			if (arg + 1 < argc) {
				hq_samples = atoi(argv[arg+1]);
				++arg;
				do_hq = true;
			}
		} else if (args == "--hqbits") {
			if (arg + 1 < argc) {
				hq_bits = atoi(argv[arg+1]);
				++arg;
				do_hq = true;
			}
		} else {
			bool failed = true;
			//check if it's a string of the form --[0-9]+x[0-9]+
			if (args.substr(0,2) == "--") {
				failed = false;
				args = args.substr(2);
				unsigned int a = 0;
				unsigned int b = 0;
				for (unsigned int i = 0; i < args.size(); ++i) {
					if (args[i] >= '0' && args[i] <= '9') {
						a *= 10;
						a += (args[i] - '0');
					} else if (args[i] == 'x') {
						args = args.substr(i+1);
						break;
					} else {
						cerr << "Couldn't parse argument as a resolution." << endl;
						failed = true;
						break;
					}
				}
				if (!failed) {
					for (unsigned int i = 0; i < args.size(); ++i) {
						if (args[i] >= '0' && args[i] <= '9') {
							b *= 10;
							b += (args[i] - '0');
						} else {
							cerr << "Couldn't parse argument as a resolution." << endl;
							failed = true;
							break;
						}
					}
				}
				if (!failed) {
					cout << "Setting resolution to " << a << "x" << b << endl;
					Graphics::screen_x = a;
					Graphics::screen_y = b;
				}
			}
			if (failed) {
				cerr << "Only recognized command-line options are '--fullscreen' and '--smallish' and '--lapscreen' and '--hqsamples' and '--hqbits'" << endl;
				return 0;
			}
		}
	}

	if (!Graphics::init(Graphics::NEED_STENCIL)) {
		cout << "Could not initialize graphics, not continuing." << endl;
		exit(1);
	}

	{ //make sure we got the right size screen:
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		if (viewport[2] != GLint(Graphics::screen_x) || viewport[3] != GLint(Graphics::screen_y)) {
			cerr << "GL viewport (" << viewport[2] << "x" << viewport[3] << ") doesn't match requested screen size." << endl;
			exit(1);
		}
	}

	SDL_EnableUNICODE(1);
	SDL_ShowCursor(SDL_DISABLE);

	//Some quick basic GL state:

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);

	if (do_hq) {
		cerr << "Starting hq mode with " << hq_samples << " samples and " << hq_bits << " bits." << endl;
		init_hq(hq_samples);
	}

	PresHack play;

	if (argc >= 2) {
		play.load(argv[1]);
	}

	play.main_loop();

	deinit_hq();

	Graphics::deinit();

	SDL_Quit();

	return 0;
}
