#include "pose_utils.hpp"

#include <Library/Library.hpp>

#include <iostream>

using std::cout;
using std::endl;

namespace Character {

void get_world_bones(Pose const &pose, WorldBones &out) {
	vector< int > parent_stack;
	vector< Quatf > orientation_stack;
	vector< Vector3f > position_stack;

	parent_stack.push_back(-1);
	orientation_stack.push_back(pose.root_orientation);
	position_stack.push_back(pose.root_position);

	out.bases.clear();
	out.tips.clear();
	out.orientations.clear();

	assert(pose.skeleton);

	for (unsigned int b = 0; b < pose.bone_orientations.size(); ++b) {
		while(!parent_stack.empty() && parent_stack.back() != pose.skeleton->bones[b].parent) {
			orientation_stack.pop_back();
			position_stack.pop_back();
			parent_stack.pop_back();
		}
		assert(!parent_stack.empty());

		//glPushMatrix();
		//glRotate(pose.bone_orientations[b]);
		orientation_stack.push_back(normalize(multiply(orientation_stack.back(), pose.bone_orientations[b])));

		//this is where draw code would be...
		out.bases.push_back(position_stack.back());
		out.orientations.push_back(orientation_stack.back());


		//glTranslate(pose.skeleton->bones[b].direction * pose.skeleton->bones[b].length);
		position_stack.push_back(position_stack.back() + rotate(make_vector< float >(pose.skeleton->bones[b].direction * pose.skeleton->bones[b].length), orientation_stack.back()));

		out.tips.push_back(position_stack.back());
		parent_stack.push_back(b);
	}
}

float world_distance(WorldBones &a, WorldBones &b) {
	assert(a.tips.size() == b.tips.size());
	float dis = 0.0f;
	for (unsigned int i = 0; i < a.tips.size(); ++i) {
		dis += length(a.tips[i] - b.tips[i]);
	}
	return dis;
}


vector< Vector3f > get_pose_positions(int motion, int frame) {
	const Library::Motion &m = Library::motion(motion);
	Character::Pose p;
	m.get_pose(frame, p);
	face_forward_and_zero_legs(p);
	Character::WorldBones wb;
	Character::get_world_bones(p, wb);
	return wb.bases;
}

void face_forward_and_zero_legs(Pose &pose) {
	pose.root_position.x = 0;
	pose.root_position.z = 0;
	//current_pose.root_orientation = multiply(-get_yaw(current_pose.root_orientation), current_pose.root_orientation);

	zero_legs(pose);

	pose.root_orientation.clear();
	Quatf rot; // .V file sideways
	rot = rotation( -(float)M_PI * 0.5f, make_vector(1.0f, 0.0f, 0.0f) );
	pose.root_orientation = multiply(rot, pose.root_orientation);
}

void zero_legs(Pose &pose) {
	for (int i = 0; i < 11; i++)
		pose.bone_orientations[i].clear();
}

}
