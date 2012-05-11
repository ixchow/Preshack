#ifndef SS_CANVAS_HPP
#define SS_CANVAS_HPP

#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include "Module.hpp"

#include <vector>

namespace {

class Layer {
public:
	Layer();
	void upload(Vector2ui size);
	std::vector< uint32_t > data;
	GLuint tex;
};

class Coef {
public:
	Coef();
	void init(Vector2ui size, float val);
	GLuint tex;
};

class SoftStacking : public Module {
public:
	SoftStacking();
	virtual ~SoftStacking();
	virtual Vector2f size();
	virtual bool handle_event(SDL_Event const &e, Vector2f mouse);
	virtual float cursor_alpha();
	virtual void update(float elapsed);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);

	float cursor_fade;
	bool have_cursor;
	
	Vector2ui img_size;
	std::vector< Layer > layers;
	std::vector< Coef > coefs;
	Box2ui dirty_region;

	Graphics::TextureRef bg_tex;
	GLuint image_tex;
	GLuint image_fb;
	GLuint stroke_tex;
	GLuint stroke_fb;
	GLuint order_tex;
	GLuint order_fb;
	GLuint temp_fb;

	PortDataContainer< std::vector< Vector3f > > layer_colors;
	PortDataContainer< std::vector< uint32_t > > closest_wanted;

	Vector2f cur_mouse;

	void render_stroke();
	std::vector< Vector3f > stroke;
	float stroke_acc;
	float brush_diameter;
	float brush_rate;

	float brush_spin_acc;

	unsigned int brush;
	std::vector< Graphics::TextureRef > brushes;

	PortDataContainer< float > alpha;
};

}

#endif //SS_CANVAS_HPP
