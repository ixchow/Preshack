#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <SDL.h>

#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

/*
 * This is the generic 'screen' class which encompases a current displayed
 * game state.
 */

#ifdef BASIS_USE_NAMESPACE
namespace Basis {
#endif //BASIS_USE_NAMESPACE

class Screen {
public:
	Screen(); //sets up screen to occupy, well, the entire screen.
	virtual ~Screen() { }

	//update the current world
	virtual void update(float elapsed_time) { }
	
	//handle the given sdl input event
	virtual void handle_event(SDL_Event const &event) { }
	
	//draw the current game world.
	virtual void draw() { }

	//internally, a screen is [-aspectf(),aspectf()]x[-1,1].
	float aspectf() const;
	double aspect() const;

	void update_screen_mouse_relative(SDL_MouseMotionEvent const &motion, Vector2f &mouse);
	void update_screen_mouse_absolute(SDL_MouseMotionEvent const &motion, Vector2f &mouse);
	void update_screen_mouse_relative(SDL_MouseMotionEvent const &motion, Vector2d &mouse);
	void update_screen_mouse_absolute(SDL_MouseMotionEvent const &motion, Vector2d &mouse);

protected:
	Box2f pixel_extents; //size in pixels.
	Box2f extents; //size and location in ambient units.
};

//run the given screen. When it calls 'quit' return.
void start_screen( Screen *screen, bool *forced_to_quit = NULL);

//call from the running screen to quit.
void quit_screen( Screen *me );

//Get screen that is 'under' this one (if any):
Screen *prev_screen( Screen *me );

//Get screen that is 'over' this one (if any):
Screen *next_screen( Screen *me );

//Add a function which is called every frame:
void add_runproc( void (*runproc)(void *), void *user_data);

//Remove a function which was called every frame:
// note: will remove *one* call matching this function pointer and data.
void remove_runproc( void (*runproc)(void *), void *user_data );

//Add a function which is called every event loop:
// (return 'true' to block event from further processing)
void add_eventproc( bool (*eventproc)(SDL_Event const &, void *), void *user_data);

//Remove a function which was called every event loop:
// note: will remove *one* call matching this function pointer and data.
void remove_eventproc( bool (*eventproc)(SDL_Event const &, void *), void *user_data );


//if a frame took less than this amount of time to render, wait.
// (This prevents the cpu from being hogged.)
extern Uint32 min_ms_per_frame;
	
//if a frame took more than this amount of time to render, pretend it
//didn't (results in a game slow-down)
extern Uint32 max_ms_per_frame;

//Average over a window of frames:
extern double avg_ms_per_frame;

#ifdef BASIS_USE_NAMESPACE
} //namespace Basis
#endif //BASIS_USE_NAMESPACE

#endif //SCREEN_HPP
