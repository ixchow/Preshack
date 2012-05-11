#include "ReflectionDistance.hpp"

#include <Library/Library.hpp>
#include <Character/Character.hpp>
#include <Character/pose_utils.hpp>

#include <sstream>

using std::ostringstream;


const unsigned int OffsetCount = 3;
const int Offsets[OffsetCount] = {1,2,3};
const float Weights[OffsetCount] = {0.8f, 0.5f, 0.3f};
/*
const unsigned int OffsetCount = 1;
const int Offsets[OffsetCount] = {1};
const float Weights[OffsetCount] = {1.0f};
*/
string const &ReflectionDistance::name() {
	static string nomen_str;
	ostringstream nomen;
	nomen << "ReflectionDistance.";
	for (unsigned int o = 0; o < OffsetCount; ++o) {
		if (o) nomen << '-';
		nomen << Offsets[o];
	}
	nomen << '.';
	for (unsigned int o = 0; o < OffsetCount; ++o) {
		if (o) nomen << '-';
		nomen << Weights[o];
	}
	nomen_str = nomen.str();
	return nomen_str;
}

ReflectionDistance::ReflectionDistance(Game2d::Fragment const &from) {
	Library::Motion const &motion = Library::motion(from.motion);
	vector< Vector3f > pivot;
	{
		Character::Pose p;
		motion.get_pose(from.end_frame-1, p);
		Character::WorldBones w;
		get_world_bones(p, w);
		pivot = w.tips;
	}
	tips.clear();
	for (unsigned int o = 0; o < OffsetCount; ++o) {
		Character::Pose p;
		motion.get_pose(from.end_frame-1-Offsets[o], p);
		Character::WorldBones w;
		get_world_bones(p, w);
		tips.push_back(w.tips);
		for (unsigned int t = 0; t < tips.back().size(); ++t) {
			tips.back()[t] = 2.0f * pivot[t] - tips.back()[t];
		}
	}
}

float ReflectionDistance::distance(Game2d::Fragment const &to) {
	Library::Motion const &motion = Library::motion(to.motion);
	float total_dis = 0.0f;
	float total_weight = 0.0f;
	for (unsigned int o = 0; o < OffsetCount; ++o) {
		Character::Pose p;
		motion.get_pose(to.start_frame-1+Offsets[o], p);
		Character::WorldBones w;
		get_world_bones(p, w);
		float dis = 0.0f;
		for (unsigned int t = 0; t < tips[o].size(); ++t) {
			Vector3f dir = tips[o][t] - w.tips[t];
			dis += dir * dir;
		}
		if (dis > 1000.0f) dis = 1000.0f;
		total_dis += dis * Weights[o];
		total_weight += Weights[0];
	}
	if (total_weight > 0.0f) {
		total_dis /= total_weight;
	}
	assert(total_dis >= 0);
	return total_dis;
}
