#ifndef GI_PAINT_HPP
#define GI_PAINT_HPP

#include "GI_StepModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include "Module.hpp"

#include <vector>

class StrokeBrush {
public:
	StrokeBrush(float softness = 0.5f, float radius = 10.0f);
	float rate; //stamps / radius
	float flow; //paint / stamp
	float radius; //radius, in pixels

	//These require re-rendering tex:
	float softness;
	GLuint get_tex();
private:
	GLuint tex; //GL_TEXTURE_2D texture for brush; updated, if needed, when you call 'get_tex()'. NOTE: always use in same GL context!
	float tex_softness;
};

class Paint : public Module {
public:
	Paint(Vector2ui const &_size);
	virtual ~Paint();
	virtual Vector2f size();
	virtual bool handle_event(SDL_Event const &e, Vector2f mouse);
	virtual float cursor_alpha();
	virtual void update(float elapsed);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);

	float cursor_fade;
	bool have_cursor;
	
	GLuint stroke_tex;
	GLuint stroke_fb;

	GLuint temp_tex;
	GLuint temp_fb;

	GLuint fb;


	Vector2f cur_mouse;

	//Painting stuff:
	void render_stroke();
	std::vector< Vector4f > acc;
	float along;
	StrokeBrush brush;
	bool erase;

	float brush_spin_acc;

	PortDataContainer< Vector3f > color;
	PortDataContainer< TexInfo > tex;
};

#endif //GI_PAINT_HPP
