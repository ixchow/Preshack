#ifndef GP_SIMPLEPAINTMODULE_HPP
#define GP_SIMPLEPAINTMODULE_HPP

#include "GP_Stroke.hpp"

#include "GP_TexViewerModule.hpp"

#include "Module.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>

#include <vector>
#include <deque>

using std::vector;
using std::deque;

////constants used when doing RGB <-> YCbCr conversion.
//const float Kb = 0.114;
//const float Kr = 0.299;

//extern unsigned int ImageWidth, ImageHeight;

//extern Graphics::ProgramObjectRef iter_shader;
//extern Graphics::ProgramObjectRef iter_shader_no_corners;

class ViewableTexture {
public:
	ViewableTexture(GLuint _tex, GLuint _fb, unsigned int _width, unsigned int _height, string _name) : tex(_tex), fb(_fb), width(_width), height(_height), name(_name) {
	}
	GLuint tex;
	GLuint fb;
	unsigned int width;
	unsigned int height;
	string name;
};

float const ColorRate = 0.99f;

float const ColorSelRad = 100.0f; //radius of color selector in pixels.

class SimplePaintModule : public Module {
public:
	SimplePaintModule(string const &filename);
	virtual ~SimplePaintModule();

	string loaded_filename;

	virtual Vector2f size();
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual float cursor_alpha();

	unsigned int ImageWidth;
	unsigned int ImageHeight;

	//This is our current version of f (may not match gradients):
	GLuint f_tex;
	GLuint f_fb;

	Vector2f mouse_pos;

	bool painting;
	float brush_size;

	Vector3f color;
	bool color_selecting;
	SmoothStroke stroke;
	void render_stroke();

	bool drew_recently;

	float brush_fade;

	Graphics::ProgramObjectRef scale_bias_subpixel_shader;
};

#endif //GP_SIMPLEPAINTMODULE_HPP
