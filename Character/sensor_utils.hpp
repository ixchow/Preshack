#ifndef CHARACTER_SENSOR_UTILS_HPP
#define CHARACTER_SENSOR_UTILS_HPP

#include <Vector/Vector.hpp>
#include <Vector/Quat.hpp>
#include <Character/Character.hpp>
#include <vector>
#include <string>
#include <fstream>
using std::vector;
using std::string;
using std::ofstream;

namespace Character {

class VirtualSensor {
public:
	VirtualSensor() : relative_position(make_vector< float, 3 >(0)) {
		relative_orientation.clear();
	}
	string bonename;
	Vector3f relative_position;
	Quatf relative_orientation;

	Quatf asf2v;
};

// calls load_ and apply_, but the two functions may be used separately.
void load_and_apply_virtual_sensors();
void load_virtual_sensors();
void apply_virtual_sensors();
void apply_virtual_sensors_only_one_motion(int motion); // used for calibrate motion

void draw_virtual_sensors(Character::Pose current_pose);
vector< float > compute_avg_sensor_diff(int motion);

void save_virtual_sensors(string filename);

// helpers
void slurp_motion_data(int motion, vector< vector< Vector3f > > &wb_bases, vector< vector< Quatf > > &wb_orientations);
void smooth_data(vector< vector< Vector3f > > &wb_bases, vector< vector< Quatf > > &wb_orientations);
void set_accelerations(int motion, bool is_v, const vector< vector< Vector3f > > &wb_bases, const vector< vector< Quatf > > &wb_orientations);
void smooth_accelerations(vector< vector< float > > &acc);


extern vector< VirtualSensor > virtual_sensors;

}
#endif
