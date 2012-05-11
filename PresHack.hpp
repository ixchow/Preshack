#ifndef PRESHACK_HPP
#define PRESHACK_HPP

#include "Mode.hpp"

#include "Item.hpp"
#include "Frame.hpp"

#include <Vector/Vector.hpp>
#include <Graphics/Graphics.hpp>

#include <vector>
#include <string>
#include <map>
using std::vector;
using std::string;
using std::map;

class Screen {
public:
	float height;
	Vector2f center;
};

class Snap {
public:
	vector< Frame * > stack;
	Frame *current;
	map< Frame *, Screen > screens;
};

class CloseSeg {
public:
	unsigned int seg;
	Vector2f seg_at;
	float seg_along;
	float seg_dis;
	Connection *seg_con;
	unsigned int point;
	Vector2f point_at;
	float point_dis;
	Connection *point_con;
};

class PresHack : public Mode {
public:
	PresHack();
	virtual ~PresHack();

	bool load(string const &filename);
	void save(string const &filename);
	void clear();
	void copy(ostream &out);
	void paste(istream &in);

	//store target, using relavance from 'rel' -- (backs up proper cameras.)
	void store_snap(Snap &target, Snap const &rel = Snap());
	virtual void handle_event(SDL_Event const &event);

	virtual void update(float elapsed_time);
	virtual void draw();

	void tick();

	map< ItemId, Item * > items;
	Frame *current;
	vector< Frame * > stack;
	enum {
		TWEEN_NONE,
		TWEEN_IN,
		TWEEN_OUT
	} tween;

	vector< Snap > snaps;
	unsigned int target_snap;
	Snap previous_snap;
	float snap_tween;
	bool snap_tweening;

	enum {
		SHOW_CURRENT,
		SHOW_ALL,
		SHOW_DEFAULT
	} show;

	Vector2f mouse; //raw, (-aspect -> aspect, -1 -> 1) mouse position
	uint8_t mouse_locked; //locked to current->tween_target -- bitmask of which buttons are down.

	Vector2f *edit_point;
	float *edit_scale;
	string *edit_string;
	unsigned int edit_string_cursor;
	string create_string;
	bool scrolling;
	bool zooming;
	bool drawing;
	bool boxing;
	Vector2f box_corner;
	Vector3f mouse_vel;
	unsigned int fix_target;

	Module *source_module;
	unsigned int source_port;

	void get_close_seg(Vector2f const &pos, CloseSeg &close);
};



#endif //PRESHACK_HPP
