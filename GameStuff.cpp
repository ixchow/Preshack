#include "GameStuff.hpp"

#include "TransitionDistance.hpp"
#include "Cache.hpp"

#include <Library/Library.hpp>
#include <Character/skin_utils.hpp>
#include <Graphics/GLSLShader.hpp>
#include <iostream>

#include <set>

using std::cout;
using std::endl;

using std::set;
using std::string;

REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);
REQUIRE_GL_EXTENSION(GL_ARB_vertex_shader);
REQUIRE_GL_EXTENSION(GL_ARB_vertex_program);

namespace Game2d {


Character::Skin &get_skin() {
	static Character::Skin *skin = NULL;
	if (skin == NULL) {
		cout << "Loading skin..." << endl;
		skin = new Character::Skin();
		Library::Motion const &m = Library::motion(0);
		if (!Character::load("player.skin", m.skeleton, *skin)) {
			skin->set_cylinders(m.skeleton);
			cout << "Couldn't load player skin." << endl;
		}
	}
	assert(skin);
	return *skin;
}

void enable_skin_shader() {
	Graphics::ShaderObjectRef skin_shader = Graphics::get_shader_object("shaders/light_vert.glsl", GL_VERTEX_SHADER_ARB);
	if (skin_shader.ref) {
		Graphics::ProgramObjectRef program = Graphics::get_program_object(skin_shader);
		if (program.ref) {
				glUseProgramObjectARB(program->handle);
		} else {
			cout << "No program!" << endl;
		}
	} else {
		cout << "No skin shader!" << endl;
	}

}

void disable_skin_shader() {
	glUseProgramObjectARB(0);
}

vector< float > tile_heights;
vector< unsigned int > spawn_locs;

vector< unsigned int > selected_motions;

vector< Fragment > fragments;
vector< Control > bins;
vector< Control > traces;

vector< float > transition_quality;
vector< float > control_quality;

vector< float > user_model;

vector< float > reward;
vector< unsigned int > policy;

float control_distance(Control const &a, Control const &b) {
	if (a.jump != b.jump) return 10.0f;
	return fabs(a.vel - b.vel);
}

float Delta = 0.1f;
float Discount = 0.9f;
float ControlQualityWeight = 1.0f;

void fragments_to_transition_quality() {
	cout << "Setting transition qualities";
	cout.flush();

	Cache::CacheFile cache("transition_quality." + TransitionDistance::name(), Cache::DEPENDS_INPUT_FRAGMENTS | Cache::DEPENDS_LIBRARY);

	transition_quality.resize(fragments.size() * fragments.size());

	if (cache.load(Cache::to_data(transition_quality))) {
		cout << " (cached) ";
	} else {
		Game2d::Control stopped;
		stopped.jump = false;
		stopped.vel = 0.0f;
		unsigned int bin_stopped = control_bin(stopped);
		for (unsigned int from = 0; from != fragments.size(); ++from) {
			TransitionDistance dis(fragments[from]);
			Game2d::Control control;
			control.jump = Library::motion(fragments[from].motion).get_control(fragments[from].start_frame).jump;
			control.vel = Library::motion(fragments[from].motion).get_control(fragments[from].start_frame).desired_velocity.z;
			unsigned int bin_start = control_bin(control);
			control.jump = Library::motion(fragments[from].motion).get_control(fragments[from].end_frame).jump;
			control.vel = Library::motion(fragments[from].motion).get_control(fragments[from].end_frame).desired_velocity.z;
			unsigned int bin_end = control_bin(control);
			bool stopped = (bin_end == bin_stopped) && (bin_start == bin_stopped);
			for (unsigned int to = 0; to != fragments.size(); ++to) {
				if (!control.jump && !stopped
				 && fragments[to].motion == fragments[from].motion
				 && fragments[to].end_frame <= fragments[from].end_frame
				 && fragments[to].start_frame + 120 >= fragments[from].end_frame) {
					transition_quality[from * fragments.size() + to] = 0.0f;
				} else {
					transition_quality[from * fragments.size() + to] = 1.0f / (1.0f + dis.distance(fragments[to]));
				}
			}
			if (from * 40 / fragments.size() != (from + 1) * 40 / fragments.size()) {
				cout << '.';
				cout.flush();
			}
		}
		cout << " (caching) "; cout.flush();
		cache.save(Cache::to_data(transition_quality));
	}
	cout << "done." << endl;
}

void fragments_and_bins_to_control_quality() {
	cout << "Setting control qualities";
	cout.flush();

	control_quality.resize(fragments.size() * bins.size());

	Cache::CacheFile cache("control_quality", Cache::DEPENDS_INPUT_FRAGMENTS | Cache::DEPENDS_LIBRARY | Cache::DEPENDS_INPUT_BINS | Cache::DEPENDS_INPUT_CONTROL_QUALITY_WEIGHT);

	if (cache.load(Cache::to_data(control_quality))) {
		cout << " (cached) "; cout.flush();
	} else {
		for (vector< Fragment >::iterator frag = fragments.begin(); frag != fragments.end(); ++frag) {
			Library::Motion const &motion = Library::motion(frag->motion);
			for (unsigned int b = 0; b < bins.size(); ++b) {
				float dis = 0.0f;
				for (unsigned int f = frag->start_frame; f < frag->end_frame; ++f) {
					Character::Control ev;
					motion.get_control(f, ev);
					Control test;
					test.jump = ev.jump;
					test.vel = ev.desired_velocity.z;
					dis += control_distance(bins[b], test);
				}
				dis /= frag->end_frame - frag->start_frame;
				control_quality[(frag - fragments.begin()) * bins.size() + b] = 1.0f / (1.0f + dis * ControlQualityWeight);
			}
			if ((frag - fragments.begin()) * 40 / fragments.size() != (frag + 1 - fragments.begin()) * 40 / fragments.size()) {
				cout << '.';
				cout.flush();
			}
		}
		cout << " (caching) "; cout.flush();
		cache.save(Cache::to_data(control_quality));
	}
	cout << "done." << endl;
}

void init_gamestuff() {
	static bool inited = false;
	if (inited) return;
	inited = true;


	//level init:
	tile_heights.clear();
	tile_heights.push_back(0.0f);
	tile_heights.push_back(0.1f);
	tile_heights.push_back(0.1f);
	tile_heights.push_back(0.1f);
	tile_heights.push_back(0.1f);
	tile_heights.push_back(0.2f);
	tile_heights.push_back(0.2f);
	tile_heights.push_back(-10.0f);
	tile_heights.push_back(0.2f);
	tile_heights.push_back(0.1f);
	tile_heights.push_back(0.0f);
	spawn_locs.clear();
	spawn_locs.push_back(0);
	spawn_locs.push_back(4);
	spawn_locs.push_back(9);

	//motion init:
	Library::init();

	selected_motions.clear();
	fragments.clear();
	bins.clear();

	set< string > motion_names;
	motion_names.insert("16_08");
	motion_names.insert("16_07");
	motion_names.insert("16_04");
	motion_names.insert("16_34");
	motion_names.insert("69_50");
	for (unsigned int ind = 0; ind < Library::motion_count(); ++ind) {
		Library::Motion const &m = Library::motion(ind);
		string file = m.filename;
		if (file.size() >= 4 && file[file.size()-4] == '.') {
			file.erase(file.size()-4,4);
		}
		if (file.size() >= 5 && motion_names.count(file.substr(file.size()-5))) {
			selected_motions.push_back(ind);
		}
	}

	//generate fragments:
	for (vector< unsigned int >::iterator i = selected_motions.begin(); i != selected_motions.end(); ++i) {
		Library::Motion const &m = Library::motion(*i);
		unsigned int len = (int)(Delta / m.skeleton->timestep);
		for (unsigned int f = 0; f + len + 1 < m.frames(); f += len) {
			fragments.push_back(Fragment());
			fragments.back().motion = *i;
			fragments.back().start_frame = f;
			fragments.back().end_frame = f + len;
			if (f + len / 2 + len + 1 < m.frames()) {
				fragments.push_back(Fragment());
				fragments.back().motion = *i;
				fragments.back().start_frame = f + len / 2;
				fragments.back().end_frame = f + len + len / 2;
			}
		}
	}
	cout << "Generated " << fragments.size() << " fragments." << endl;

	//Create bins:
	bins.clear();
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel =-1.0f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel =-0.1f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 0.0f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 0.1f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 0.5f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 1.0f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 2.0f;
	bins.push_back(Control()); bins.back().jump = false; bins.back().vel = 3.0f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel =-1.0f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel =-0.1f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 0.0f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 0.1f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 0.5f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 1.0f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 2.0f;
	bins.push_back(Control()); bins.back().jump = true; bins.back().vel = 3.0f;

	//transition qualities matrix:
	transition_quality.clear();
	fragments_to_transition_quality();

	//control qualities matrix:
	control_quality.clear();
	fragments_and_bins_to_control_quality();

	//user model (clear):
	user_model.clear();
	user_model.resize(bins.size() * bins.size(), 0.0f);

	//clear reward-n-policy:
	reward.clear();
	reward.resize(fragments.size() * bins.size(), 0.0f);

	policy.clear();
	policy.resize(fragments.size() * bins.size(), 0);

	{//do policy cache check..
		Cache::CacheFile cache("policy", 0);

		if (cache.load(Cache::to_data(policy))) {
			cout << "loaded cached policy" << endl;
		} else {
			cout << "failed to load cached policy" << endl;
			for (unsigned int i = 0; i < policy.size(); ++i) {
				policy[i] = (i + 1) % fragments.size();
			}
		}
	}
}

void do_frequency_counts() {
	user_model.clear();
	user_model.resize(bins.size() * bins.size(), 0.0f);
	if (traces.empty()) return;
	unsigned int last_bin = control_bin(traces[0]);
	for (unsigned int i = 1; i < traces.size(); ++i) {
		unsigned int bin = control_bin(traces[i]);
		user_model.at(last_bin * bins.size() + bin) += 1.0f;
		last_bin = bin;
	}
	for (unsigned int b = 0; b < bins.size(); ++b) {
		float sum = 0.0f;
		for (unsigned int dest = 0; dest < bins.size(); ++dest) {
			sum += user_model.at(b * bins.size() + dest);
		}
		if (sum == 0.0f) {
			sum = 1.0f;
			user_model.at(b * bins.size() + b) = 1.0f;
		}
		float fac = 1.0f / sum;
		for (unsigned int dest = 0; dest < bins.size(); ++dest) {
			user_model.at(b * bins.size() + dest) *= fac;
		}
	}
}

static vector< float > new_reward;
void step_iteration() {
	if (new_reward.size() != reward.size()) {
		new_reward.resize(reward.size());
	}
	for (unsigned int f = 0; f < fragments.size(); ++f) {
		for (unsigned int c = 0; c < bins.size(); ++c) {
			new_reward.at(f * bins.size() + c) = 0.0f;
			for (unsigned int fp = 0; fp < fragments.size(); ++fp) {
				float r = 0.0f;
				for (unsigned int cp = 0; cp < bins.size(); ++cp) {
					r += user_model.at(c * bins.size() + cp) * reward.at(fp * bins.size() + cp);
				}
				r *= Discount; //discount expected future reward.
				r += transition_quality.at(f * fragments.size() + fp) * control_quality.at(fp * bins.size() + c); //reward for choosing fp.
				if (r > new_reward.at(f * bins.size() + c)) {
					new_reward.at(f * bins.size() + c) = r;
				}
			}
		}
	}
	reward = new_reward;
}

void pick_policy() {
	policy.clear();
	policy.resize(fragments.size() * bins.size(), 0);
	for (unsigned int f = 0; f < fragments.size(); ++f) {
		for (unsigned int c = 0; c < bins.size(); ++c) {
			float best = 0.0f;
			for (unsigned int fp = 0; fp < fragments.size(); ++fp) {
				float r = 0.0f;
				for (unsigned int cp = 0; cp < bins.size(); ++cp) {
					r += user_model.at(c * bins.size() + cp) * reward.at(fp * bins.size() + cp);
				}
				r *= Discount; //discount expected future reward.
				r += transition_quality.at(f * fragments.size() + fp) * control_quality.at(fp * bins.size() + c); //reward for choosing fp.
				if (r > best) {
					best = r;
					policy.at(f * bins.size() + c) = fp;
				}
			}
		}
	}
}

void save_policy() {
	Cache::CacheFile cache("policy", 0);
	cache.save(Cache::to_data(policy));
	cout << "Saved policy to cache." << endl;

}


};
