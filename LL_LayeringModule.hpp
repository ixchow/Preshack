#ifndef LL_LAYERINGMODULE_HPP
#define LL_LAYERINGMODULE_HPP

#include "LL_ListGraph.hpp"
#include "LL_Obj.hpp"

#include "Module.hpp"

#include <Graphics/Graphics.hpp>

#include <string>
#include <vector>

#ifndef WINDOWS
#include <stdint.h>
#endif

using std::vector;
using std::string;

const float BinWidth = 1.0f;
const float BinHeight = 0.45f;

class DialogBin {
public:
	DialogBin(unsigned int _layer, string _name)
		: old_pos(make_vector(0.0f, 0.0f)),
		old_show(make_vector(0.0f, 0.0f, 1.0f)),
		new_pos(make_vector(0.0f, 0.0f)),
		new_show(make_vector(0.0f, 0.0f, 1.0f)),
		layer(_layer),
		active(false),
		name(_name),
		arrows(0.0f) {
	}
	Vector2f old_pos;
	Vector3f old_show;
	Vector2f new_pos;
	Vector3f new_show;
	Vector2f get_pos(float tween) const {
		tween *= 3.0f;
		if (tween > 1.0f) tween = 1.0f;
		float bump = 0.0f;
		if (old_pos.y < new_pos.y) {
			bump = sinf(tween * 1.5f * M_PI) + tween;
			bump *= 0.05f;
		}
		return old_pos + (2.0f - powf(2.0f, 1.0f - tween)) * (new_pos - old_pos) + make_vector(-bump, 0.0f);
	}
	Vector3f get_show(float tween, Vector3f all) const {
		if (old_show == new_show) {
			return new_show;
		}
		Vector3f a = old_show + tween * (all - old_show);
		Vector3f b = all + tween * (new_show - all);
		return a + tween * (b - a);
	}
	Box2f get_box(float tween) const {
		Box2f ret;
		ret.min = get_pos(tween);
		ret.max = ret.min;
		ret.max.x += 0.5f;
		ret.min.x -= 0.5f;
		ret.max.y += BinHeight * 0.5f / BinWidth;
		ret.min.y -= BinHeight * 0.5f / BinWidth;
		return ret;
	}
	Box2f get_tex_box(float tween) const {
		Box2f ret;
		ret.min = get_pos(tween);
		ret.min.x -= 0.5f;
		ret.max = ret.min;
		ret.max.x += BinHeight / BinWidth;
		ret.max.y += BinHeight * 0.5f / BinWidth;
		ret.min.y -= BinHeight * 0.5f / BinWidth;
		return ret;
	}
	unsigned int layer;
	bool active;
	string name;
	float arrows;
	float zoom_out;
};

class LayeringModule : public Module {
public:
	LayeringModule();
	virtual ~LayeringModule();

	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	void add_layer(string filename);
	bool update_regions(bool can_fail = false);
	void update_image();


	unsigned int width, height;
	vector< vector< uint32_t > > layers;
	vector< Vector3f > colors; //layer colors...
	unsigned int fixed_layers;
	vector< GLuint > textures;
	vector< uint32_t > bits;
	vector< uint32_t > tags;
	vector< uint32_t > tag_colors; //used for show_tags.
	bool show_tags;
	//vector< vector< unsigned int > > tag_stacking;
	//vector< vector< unsigned int > > tag_neighbors;
	ListGraph stacking;
	vector< uint32_t > image;
	GLuint image_tex;
	bool image_dirty;
	GLuint get_tags_tex();
private:
	GLuint tags_tex;
	bool tags_dirty;
public:
	int acc_move_x, acc_move_y;
	//Layers dialog:
	Vector< unsigned int, 2 > dialog_point;
	unsigned int current_layer;
	void update_dialog(); //...for new point.

	//For pretty little bins:
	vector< DialogBin > bins;
	float dialog_tween;
	Vector2f mouse_pos;
	Box2i dialog_box;

	//For animations:
	unsigned int current_frame;
	unsigned int total_frames;
	void set_frame(unsigned int frame);
	void update_anim_tags();
	vector< vector< vector< uint32_t > > > anim_layers;
	vector< vector< string > > anim_filenames;
	vector< vector< uint32_t > > anim_tags;

	//For 3d (Now it's realllly complicated.)
	void add_model(string filename);
	ObjModel model;
	Vector3f camera;
	Vector3f new_camera;
	Vector3f target;
	void render_model();
	bool model_dirty;
	vector< GLuint > model_layers;

	//How to handle failure:
	bool allow_fail;

	//don't show background:
	bool no_background;
	PortDataContainer< LayeringModule * > this_port;
	PortDataContainer< float > anim_pos;
	float last_anim_pos;

	void draw_arrow(Vector2f c1, Vector2f c2);
};

#endif //LL_LAYERINGMODULE_HPP
