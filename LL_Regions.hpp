#ifndef LL_REGIONS_HPP
#define LL_REGIONS_HPP

#include "Module.hpp"

class LayeringModule;

const float ShaftWidth = 0.10f;
const float BoxHeight = 0.08f;
const float ShaftInset = 0.01f;
const float ArrowWidth = 0.14f;
const float ArrowHeight = 0.10f;

const float ActiveScale = 1.5f;
const float StateWidth = 0.15f;
const float StateHeight = 0.075f;

class LayerBox {
public:
	LayerBox() : layer((unsigned)-1), color(make_vector(0.0f, 0.0f, 0.0f, 0.0f)), pos(0.0f), scale(0.0f), fade(0.0f), target_pos(0.0f), target_disp(false) { }

	unsigned int layer;

	Vector4f color;

	float pos;
	float scale;
	float fade;

	void update_pos(float elapsed_time);

	float target_pos;
	bool target_disp;

	Box2f get_box() const {
		Box2f ret;
		ret.min.x = scale * (-ShaftWidth * 0.5f + ShaftInset);
		ret.max.x = scale * ( ShaftWidth * 0.5f - ShaftInset);
		ret.min.y = pos - scale * 0.5f * BoxHeight;
		ret.max.y = pos + scale * 0.5f * BoxHeight;
		return ret;
	}

	bool operator<(LayerBox const &other) const {
		if (!target_disp && other.target_disp) return true;
		if (target_disp && !other.target_disp) return false;
		return target_pos < other.target_pos;
	}
};

class GraphArrow {
public:
	GraphArrow();
	unsigned int ind;
	Vector2f loc;
	void update(float elapsed_time);
	bool update_location(LayeringModule &layering, Vector2f loc);
	vector< LayerBox > boxes;
	bool operator<(GraphArrow const &other) const {
		return loc.y > other.loc.y;
	}
};


const unsigned int ShowX = 0x10;
const unsigned int ShowY = 0x20; //layer above X
const unsigned int ShowM = 0x40; //show layer under X.
const unsigned int ShowT = 0x80;
enum {
	INIT = 0x0,
	CHECK = 0x1 | ShowX | ShowT,
	CHECK_RETURN = 0x2,
	WHILE = 0x3 | ShowX | ShowT,
	WHILE_RETURN = 0x4,
	SWAP = 0x5,
	SWAP_ACTION = 0x6 | ShowX | ShowY,
	SWAP_AFTER = 0x7,
	RECURSE = 0x8,
	RECURSE_AFTER = 0xa,
};

class FlipState {
public:
	FlipState(unsigned int _r, unsigned int _x, unsigned int _t) : region(_r), x(_x), t(_t), state(INIT), color_x(make_vector< float, 4 >(1.0f)), color_t(make_vector< float, 4 >(1.0f)) { }
	unsigned int region;
	unsigned int x;
	unsigned int t;
	unsigned int state;
	Vector4f color_x;
	Vector4f color_t;
	Box2f pos; //left and right port positions?
};

class LayersRegions : public Module {
public:
	LayersRegions();
	virtual ~LayersRegions();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	vector< GraphArrow > arrows;

	vector< FlipState * > stack;

	void start_execution(unsigned int region, unsigned int x, unsigned int t);
	void clear_execution();
	void step_execution();

	bool draw_graph;
	float draw_graph_tween;

	GraphArrow *dragging;

	//for starting execution...
	unsigned int last_region;
	unsigned int last_layer;

	Vector2f mouse_pos;
	PortDataContainer< LayeringModule * > layering;
	PortDataContainer< FlipState * > top_state;
};

#endif //LL_REGIONS_HPP
