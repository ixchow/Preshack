#ifndef GAMESTUFF_HPP
#define GAMESTUFF_HPP

#include <Vector/Vector.hpp>
#include <Character/Skin.hpp>

#include <vector>
#include <assert.h>

namespace Game2d {

using std::vector;

//---------------------------------------------
//Current game level stuff (the level, mainly)
extern vector< float > tile_heights;
extern vector< unsigned int > spawn_locs;

const float PlayerRadius = 0.4f;
const float PlayerHeight = 2.5f;
const float StepHeight = 0.11f;

class Frame {
public:
	unsigned int motion;
	unsigned int frame;
};

class LevelStatus {
public:
	Vector2f pos;
	bool flip;
	vector< Vector2f > spheres;
};

Character::Skin &get_skin();
void enable_skin_shader();
void disable_skin_shader();

//---------------------------------------------
//RCFMF stuff [fragments, control bins, ... policy...]

class Control {
public:
	Control() : vel(0.0f), jump(false) { }
	float vel;
	bool jump;
};

class Fragment {
public:
	unsigned int motion;
	unsigned int start_frame;
	unsigned int end_frame;
};

extern vector< unsigned int > selected_motions;

extern vector< Fragment > fragments;
extern vector< Control > bins;

extern vector< Control > traces;

extern vector< float > transition_quality;
extern vector< float > control_quality;

extern vector< float > user_model;

inline float get_transition_quality(unsigned int from, unsigned int to) {
	return transition_quality.at(from * fragments.size() + to);
}

inline float get_control_quality(unsigned int frag, unsigned int bin) {
	return control_quality.at(frag * bins.size() + bin);
}

inline float get_transition_probability(unsigned int from, unsigned int to) {
	return user_model.at(from * bins.size() + to);
}


float control_distance(Control const &a, Control const &b);

inline unsigned int control_bin(Control const &control) {
	assert(!bins.empty());
	if (bins.empty()) return 0;
	float dis = control_distance(bins[0], control);
	unsigned int best = 0;
	for (unsigned int b = 1; b < bins.size(); ++b) {
		float test = control_distance(bins[b], control);
		if (test < dis) {
			dis = test;
			best = b;
		}
	}
	return best;
}

extern vector< float > reward; //fragments x bins...
extern vector< unsigned int > policy; //fragments x bins...

inline float get_reward(unsigned int fragment, unsigned int control_bin) {
	return reward.at(fragment * bins.size() + control_bin);
}

inline unsigned int next_fragment(unsigned int current_fragment, unsigned int control_bin) {
	return policy.at(current_fragment * bins.size() + control_bin);
}

extern float Delta;
extern float Discount;
extern float ControlQualityWeight;

void init_gamestuff();

void do_frequency_counts();

void step_iteration();

void pick_policy();

void save_policy();

}





#endif //GAMESTUFF_HPP
