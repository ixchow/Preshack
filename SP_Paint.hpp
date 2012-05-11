#ifndef PAINT_HPP
#define PAINT_HPP

#include "Module.hpp"

#include <Graphics/Graphics.hpp>

#include <vector>

using std::vector;
using std::map;

class SP_Paint : public Module {
public:
	SP_Paint(Vector2ui size, uint32_t *a_data, uint32_t *b_data);
	~SP_Paint();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float time);
	virtual bool handle_event(SDL_Event const &, Vector2f mouse);
	virtual float cursor_alpha();
	
	GLuint a_tex, b_tex, order_tex;
	GLuint order_fb;

	Vector2ui canvas_size;

	float draw_cursor;
	Vector2f old_mouse;

	float stroke_rad;
	Vector3f stroke_color;
	vector< Vector3f > stroke_queue;
	float stroke_acc;
	unsigned int stroke_brush;
	vector< GLuint > brushes;
	float stroke_rate;
	float stroke_pressure;

	//render stroke_queue into stroke, clear stroke queue.
	void render_stroke();
	//merge current stroke into.. what you wanted to merge it into.. I guess.
	void finalize_stroke();
};

#endif //PAINT_HPP
