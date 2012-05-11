#ifndef SS_OPT_CANVAS_HPP
#define SS_OPT_CANVAS_HPP

#include "SS_optimizer.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include "Module.hpp"

#include <vector>


class Const {
public:
	vector< uint32_t > valid_orders;
	uint32_t col;
};

class OptStacking : public Module {
public:
	OptStacking(std::string base_name = "");
	virtual ~OptStacking();
	virtual Vector2f size();
	virtual bool handle_event(SDL_Event const &e, Vector2f mouse);
	virtual float cursor_alpha();
	virtual void update(float elapsed);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);

	float cursor_fade;
	bool have_cursor;
	
	std::string base_name;

	Vector2ui img_size;
	std::vector< Layer > layers;
	std::vector< Const > constraints;

	std::vector< uint32_t > free_colors;

	SolveStatus *solve;
	void start_solve(bool store_coefs);

	GLuint soln_tex;
	Vector2ui soln_size;
	uint32_t soln_serial;
	GLuint cons_tex;
	GLuint cons_fb;
	GLuint brush_tex; //just a circle

	PortDataContainer< std::vector< Vector3f > > layer_colors;
	PortDataContainer< std::vector< uint32_t > > closest_wanted;

	Vector2f cur_mouse;

	void render_stroke();
	std::vector< Vector3f > stroke;
	float stroke_acc;
	float brush_diameter;
	float brush_rate;

	float brush_spin_acc;

	float stipple_ofs;
};

#endif //SS_OPT_CANVAS_HPP
