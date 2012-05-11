#ifndef POISSONTIMINGMODULE_HPP
#define POISSONTIMINGMODULE_HPP
#include "Module.hpp"
#include "GI_StepModule.hpp"

#include <Graphics/Texture.hpp>

class PoissonTimingModule : public Module {
private:
	void dealloc_tex();
	void alloc_tex();
	void clear_tex();
public:
	PoissonTimingModule();
	virtual ~PoissonTimingModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
private:
	void draw_stroke();
	void draw_distance();
	void do_gaussian(); //for timing comparison
public:
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	GLuint cons_fb; //framebuffer bound to:
	GLuint cons_tex; //color texture
	GLuint cons_rb;  //depth renderbuffer
	GLuint soln_fb; //solution fb bound to:
	GLuint soln_tex;
	GLuint temp_fb; //temp fb bound to:
	GLuint temp_tex;

	bool run;
	bool step;

	float jacobi_scale;

	int show;
	enum {
		SHOW_CONS,
		SHOW_SOLN
	};

	Vector3f color_a;
	Vector3f color_b;

	vector< Vector2f > stroke;

	PortDataContainer< unsigned int > iter_count;
};

#endif //POISSONTIMINGMODULE_HPP
