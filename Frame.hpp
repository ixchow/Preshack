#ifndef FRAME_HPP
#define FRAME_HPP

#include "Item.hpp"
#include "String.hpp"
#include "Module.hpp"

#include <Vector/Box.hpp>
#include <Graphics/Graphics.hpp>

#include <vector>

using std::vector;

typedef unsigned int FrameId;

class Connection {
public:
	Connection() :
		from_module(NULL), to_module(NULL),
		from_port((unsigned)-1), to_port((unsigned)-1),
		have_start(false), start(make_vector(0.0f, 0.0f)),
		have_end(false), end(make_vector(0.0f, 0.0f)) {
	}
	Module *from_module;
	Module *to_module;
	unsigned int from_port;
	unsigned int to_port;
	bool have_start;
	Vector2f start;
	bool have_end;
	Vector2f end;
	vector< Vector2f > points;
};

class Frame : public Item {
public:
	Frame();
	virtual ~Frame() {}

	vector< Item * > items;
	vector< Connection > connections;
	vector< Module * > modules; //subset of items.

	//background gradient:
	Vector3f top_color;
	Vector3f bottom_color;
	GLuint grad_tex;

	Box2f area; //internal area.
	float screen_height; //screen height. (in 'area')
	Vector2f screen_center; //camera position. (in 'area')


private:
	float tween_speed(Vector2f const &from_pos, Vector2f const &from_size, Vector2f const &to_pos, Vector2f const &to_size) const {
		float from_rad = length(from_size) * 0.5f;
		float to_rad = length(to_size) * 0.5f;

		float trans = length(to_pos - from_pos);
		if (from_rad > to_rad) {
			trans += from_rad - to_rad;
		} else {
			trans += to_rad - from_rad;
		}

		float half_rad = 0.5f * (from_rad + to_rad);
		float units_per_scale = 1.0f;
		const float min_units_per_scale = 0.1f;
		const float max_units_per_scale = 2.0f;
		if (half_rad * min_units_per_scale > trans) {
			units_per_scale = min_units_per_scale;
		} else if (half_rad * max_units_per_scale < trans) {
			units_per_scale = max_units_per_scale;
		} else {
			units_per_scale = trans / half_rad;
		}
		return 1.0f / units_per_scale;

	}
public:
	float tween_in; //moving 'in' to this. 0.0f -> preview, 1.0f -> here.
	float tween_in_speed() const {
		if (!tween_target) return 1.0f;
		Vector2f pos = preview.center();
		Vector2f size = preview.size();
		Vector2f camera_pos = screen_center;
		Vector2f camera_size = make_vector(screen_height * Graphics::aspectf(), screen_height);
		return tween_speed(pos, size, camera_pos, camera_size);

	}

	float tween; //moving to tween_target. 0.0f -> here, 1.0f -> there.
	float tween_speed() const {
		if (!tween_target) return 1.0f;
		Vector2f pos = tween_target->position;
		Vector2f size = tween_target->scale * tween_target->size();
		Vector2f camera_pos = screen_center;
		Vector2f camera_size = make_vector(screen_height * Graphics::aspectf(), screen_height);
		return tween_speed(pos, size, camera_pos, camera_size);
	}
	Item *tween_target;

	vector< vector< Vector2f > > scribbles;
	Box2f box;
	bool show_box;
	float box_fade;

	Box2f preview; //area shown when 'outside'
	virtual Vector2f size(); //returns 'preview' (with some tweening tweaks)
	vector< Box2f > previews;

	vector< Vector2f > fixes;

	//recurse -> draw lower layers
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);

	void update_modules(float elapsed_time);

	//set tween_target based on mouse position
	//   -- note: mouse is in 'external' / screen_viewport coordinates!
	void set_target(Box2f viewport, Box2f screen_viewport, Vector2f mouse);
	//move mouse to internal coordinates.
	Vector2f get_mouse(Box2f viewport, Box2f screen_viewport, Vector2f mouse);
	void get_boxes(Box2f &internal, Box2f &external);
	void correct_camera();

};

#endif //FRAME_HPP
