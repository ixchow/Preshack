#ifndef CHARACTER_POSE_UTILS_HPP
#define CHARACTER_POSE_UTILS_HPP

#include "Character.hpp"

namespace Character {

class WorldBones {
public:
	vector< Vector3f > bases;
	vector< Vector3f > tips;
	vector< Quatf > orientations;
};

void get_world_bones(Pose const &pose, WorldBones &out);

float world_distance(WorldBones &a, WorldBones &b);

void lower_to_ground(Pose const &pose);

vector< Vector3f > get_pose_positions(int motion, int frame);
void face_forward_and_zero_legs(Pose &pose);
void zero_legs(Pose &pose);

};

#endif //CHARACTER_POSE_UTILS_HPP
