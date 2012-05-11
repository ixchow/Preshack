#ifndef FFTPAINTMODULE_HPP
#define FFTPAINTMODULE_HPP
#include "Module.hpp"
#include "GradientStuff.hpp"

class FFTPaintModule : public Module {
public:
	FFTPaintModule() {

		in_ports.push_back(&color);
		color.name = "color";
		color.position = make_vector(-1.0f, 0.0f);
		color() = make_vector(0.5f, 0.5f, 0.5f);


		alloc_tex(SimpleWidth, SimpleHeight, f_tex);
		alloc_fb(f_tex, f_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gx_tex); //cyclic, same
		alloc_fb(gx_tex, gx_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gx_temp_tex); //cyclic, same
		alloc_fb(gx_temp_tex, gx_temp_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gy_tex); //cyclic, same
		alloc_fb(gy_tex, gy_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gy_temp_tex); //cyclic, same
		alloc_fb(gy_temp_tex, gy_temp_fb);

		alloc_tex(SimpleWidth, SimpleHeight, f_temp_tex);
		alloc_fb(f_temp_tex, f_temp_fb);

		alloc_tex(SimpleWidth, SimpleHeight, rhs_tex);
		alloc_fb(rhs_tex, rhs_fb);

		alloc_tex(SimpleWidth, SimpleHeight, gx_clone_tex);
		alloc_tex(SimpleWidth, SimpleHeight, gy_clone_tex);

		alloc_tex(SimpleWidth, SimpleHeight, gx_residual_tex);
		alloc_fb(gx_residual_tex, gx_residual_fb);

		alloc_tex(SimpleWidth, SimpleHeight, gy_residual_tex);
		alloc_fb(gy_residual_tex, gy_residual_fb);

		freq_edit = false;

		//Textures to store 'low frequency' stuff
		alloc_tex(SimpleWidth, SimpleHeight, gx_lf_tex);
		alloc_fb(gx_lf_tex, gx_lf_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gy_lf_tex);
		alloc_fb(gy_lf_tex, gy_lf_fb);
		// + stuff we really want to edit.
		alloc_tex(SimpleWidth, SimpleHeight, gx_edit_tex);
		alloc_fb(gx_edit_tex, gx_edit_fb);
		alloc_tex(SimpleWidth, SimpleHeight, gy_edit_tex);
		alloc_fb(gy_edit_tex, gy_edit_fb);

		show = SHOW_F;
		grad_dirty = true;

		clone = false;
		painting = false;
		clone_offset = clone_point = make_vector(0.0f, 0.0f);
		mouse_pos = mouse_dir = make_vector(0.0f, 0.0f);
		brush_size = 10.0f;

		filename = "";

		relax_mode = false;
	}
	virtual ~FFTPaintModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);
	//Important data:
	GLuint f_tex, gx_tex, gy_tex, rhs_tex;
	GLuint f_fb, gx_fb, gy_fb, rhs_fb;
	GLuint gx_temp_tex, gy_temp_tex;
	GLuint gx_temp_fb, gy_temp_fb;
	GLuint f_temp_tex, f_temp_fb;
	GLuint gx_residual_tex, gx_residual_fb;
	GLuint gy_residual_tex, gy_residual_fb;

	//cloning?
	bool clone;
	bool painting;
	GLuint gx_clone_tex, gy_clone_tex;
	Vector2f clone_offset;
	Vector2f clone_point;
	bool freq_edit; //edit a specific frequency band.
	//Store texture frequencies below desired band:
	GLuint gx_lf_tex, gy_lf_tex;
	GLuint gx_lf_fb, gy_lf_fb;
	//We'll edit this band:
	GLuint gx_edit_tex, gy_edit_tex;
	GLuint gx_edit_fb, gy_edit_fb;
	//painting:
	Vector2f mouse_pos;
	Vector2f mouse_dir;
	float brush_size;

	bool relax_mode;
	
	bool grad_dirty;
	unsigned int show;
	enum {
		SHOW_F,
		SHOW_RHS,
		SHOW_GX,
		SHOW_GXR,
		SHOW_GY,
		SHOW_GYR,
	};
	//helpers:
	string filename;
	void calc_residual_gradients();
	void load();
	void grad_line(Vector2f from, Vector2f to, float radius, Vector2f dir, Vector4f c);
	void build_edit_tex();
	void collapse_edit_tex();

	PortDataContainer< Vector3f > color;
};

#endif //FFTPAINTMODULE_HPP
