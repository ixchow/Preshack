#ifndef GP_GENERALPAINTMODULE_HPP
#define GP_GENERALPAINTMODULE_HPP

#include "GP_Integrator.hpp"

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

class BlendMode {
public:
	BlendMode(string const &_name, string const &filename);
	string name;
	Graphics::ProgramObjectRef gx_shader;
	Graphics::ProgramObjectRef gy_shader;
};

float const ColorRate = 0.99f;

float const EdgeAlongStep = 1.0f; //how much we move along edge texture per unit length.

float const ColorSelRad = 100.0f; //radius of color selector in pixels.

class GeneralPaintModule : public Module {
public:
	GeneralPaintModule(string const &filename, Integrator *_integrator);
	virtual ~GeneralPaintModule();

	string loaded_filename;

	virtual Vector2f size();
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual float cursor_alpha();

	unsigned int ImageWidth;
	unsigned int ImageHeight;

	Integrator *integrator;

	//void test_integrator();

	bool grad_dirty;
	bool single_stepping;
	bool step;
	bool dump_frame;

	//These are the actual gradients we're integrating:
	GLuint gx_tex, gy_tex;
	GLuint gx_fb, gy_fb;

	//This is our current version of f (may not match gradients):
	GLuint f_tex;
	GLuint f_fb;

	//Keep original around (for integrator debugging, mainly):
	GLuint f_orig_tex;

	//For clearing:
	vector< Vector3f > f_orig;

	//load image, process gradients.
	void load(std::string const &name);

	//Graphics::ProgramObjectRef YCbCr_shader;
	//Graphics::ProgramObjectRef gx_residual_shader;
	//Graphics::ProgramObjectRef gy_residual_shader;
	
	//For ATI_HACK:
	Graphics::ProgramObjectRef interpolate_shader;

	//used when copying the current viewable to the screen:
	Graphics::ProgramObjectRef scale_bias_shader;
	Graphics::ProgramObjectRef scale_bias_subpixel_shader;
	Graphics::ProgramObjectRef compare_shader;
	bool compare;
	bool color_adapt;
	bool super_scale;
	Vector3f center_color_acc;
	Vector3f max_color_acc;
	Vector3f min_color_acc;

	Vector2f mouse_pos;

	enum {
		PAINT_MODE = 0,
		CLONE_MODE = 1,
		EDGE_MODE = 2,
		NUM_MODES = 3
	};
	unsigned int mode;

	bool painting;
	float brush_size;
	GLuint gx_src_tex, gy_src_tex; //used to get around broken blending.

	vector< string > help; //for help mode!
	bool help_on;

	//set based on both mode and action:
	Stroke *current_stroke;
	bool show_stroke_line;

	bool dragging_stroke;
	Vector2f drag_start;
	vector< Vector2f > drag_start_points;

	//PAINT_MODE stuff:
	Vector3f paint_color;
	bool paint_color_selecting;
	SmoothStroke paint_stroke;
	void render_paint_stroke();

	//EDGE_MODE stuff:
	bool edge_selecting;
	bool edge_detail;
	unsigned int edge_rad;
	unsigned int edge_steps;
	SmoothStroke example_edge_stroke; //copy edge from
	SmoothStroke edge_stroke; //copy edge to
	GLuint edge_sample_tex, edge_sample_fb;
	GLuint edge_perp_tex, edge_perp_fb; //perp tex is edge_steps x (edge_rad*2-1)
	GLuint edge_tan_tex, edge_tan_fb; //tan is (edge_steps-1) x edge_rad*2
	Graphics::ProgramObjectRef edge_gradient_shader;
	void grab_current_edge();
	void render_edge_stroke();

	//CLONE_MODE stuff:
	Vector2f clone_point;
	SmoothStroke clone_stroke;
	void render_clone_stroke();

	//for drawing edges:
	GLuint gx_depth_rb;
	GLuint gy_depth_rb;
	Graphics::ProgramObjectRef sum_shader;

	unsigned int current_blend_mode;
	vector< BlendMode > blend_modes;

	void render_stroke(Stroke const &stroke, GLuint tan_tex, GLuint perp_tex, unsigned int tex_rad, unsigned int steps_along, float brush_rad, bool clone_orient);

	vector< ViewableTexture > viewable_textures;
	unsigned int current_viewing_texture;

	//PORTS!
	PortDataContainer< TexPacket > gx_port;
	PortDataContainer< TexPacket > gy_port;
	PortDataContainer< TexPacket > edge_sample_port;
	PortDataContainer< TexPacket > edge_tan_port;
	PortDataContainer< TexPacket > edge_perp_port;
	bool drew_recently;

	float brush_fade;
	float text_fade;
};

#endif //GP_GENERALPAINTMODULE_HPP
