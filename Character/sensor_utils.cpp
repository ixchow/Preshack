#include "sensor_utils.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <Library/Library.hpp>
#include <Character/pose_utils.hpp>
#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>
#include <Vector/QuatGL.hpp>
#include <math.h>


using std::cout;
using std::endl;
using std::ofstream;
using std::ifstream;

namespace Character {

vector< VirtualSensor > virtual_sensors;

vector< float > compute_avg_sensor_diff(int motion) {
	const Library::Motion &m = Library::motion(motion);
	vector< vector< float > > sen = m.get_sensors();
	vector< vector< float > > acc = m.accelerations;
	assert(sen.size() == acc.size());
	assert(sen[0].size() == acc[0].size());
	assert(sen.size() != 0);

	vector< float > diffs(sen[0].size(), 0.0f);
	assert(sen.size() > 400);
	for (unsigned int f = 200; f < 2000; f++) {
		for (unsigned int s = 0; s < sen[0].size(); s++)
			diffs[s] += (sen[f][s] - acc[f][s]) * (sen[f][s] - acc[f][s]);
	}
	for (unsigned int s = 0; s < sen[0].size(); s++)
		diffs[s] = sqrt(diffs[s] / sen.size());
	return diffs;
}

#define FPS 120
void draw_virtual_sensors(Character::Pose current_pose) {
	glLineWidth(4);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.05f, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.05f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.05f);
	glEnd();
	glLineWidth(1);


	//Quatf y2z; y2z.x = -.5; y2z.y = -.5; y2z.z = -.5; y2z.w = .5;

	Character::WorldBones wb;
	get_world_bones(current_pose, wb);
	for (unsigned int s = 0; s < Character::virtual_sensors.size(); ++s) {
		int bone = current_pose.skeleton->get_bone_by_name(Character::virtual_sensors[s].bonename);
		assert(bone != -1);

		Quatf wborient;
		string f = current_pose.skeleton->filename;
		if (f[f.size() - 1] == 'K' || f[f.size() - 1] == 'k')
			wborient = wb.orientations[bone];
		else if (f[f.size() - 1] == 'F' || f[f.size() - 1] == 'f')
	   		wborient = multiply(wb.orientations[bone], Character::virtual_sensors[s].asf2v);
		else
			assert(0);

		// global position and rotation of the sensor
		Vector3f pos = wb.bases[bone] + rotate(Character::virtual_sensors[s].relative_position, wborient);


		Quatf rot = multiply(wborient, Character::virtual_sensors[s].relative_orientation);
		//Note:
		//accel += gravity_z;
		//Vector3f local_accel = rotate(accel, conjugate(rot));

		static GLUquadric *quad = NULL;
		if (quad==NULL) quad = gluNewQuadric();

	
		glPushMatrix();
		glTranslate(pos);
		glRotate(rot);

		glColor3f(0.0, 0.0, 0.0);
		gluDisk(quad, 0, .05, 8, 8);
		glLineWidth(4);
		glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.1f, 0.0f, 0.0f);
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.1f, 0.0f);
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.1f);
		glEnd();
		glLineWidth(1);
		glPopMatrix();
	}
}

void load_and_apply_virtual_sensors() {
	load_virtual_sensors();
	apply_virtual_sensors();
}

void load_virtual_sensors() {
	cout << "Loading skeleton sensor locations" << endl;

	if (Library::motion_count()==0) {
		cout << "No motions." << endl;
		return;
	}
	
	// print out a list of the bones
	ofstream outFile("skel.dat");
	assert(outFile);
	const Library::Motion &m = Library::motion(0);
	for (unsigned int b = 0; b < m.skeleton->bones.size(); b++)
		outFile << b << " " << m.skeleton->bones[b].name << endl;
	outFile.close();

	// read in the sensor set
	ifstream inpFile("sensor_locations.txt");
	if (! inpFile ) {
		cout << "Couldn't open input file sensor_locations.txt" << endl;
		return;
	}
	virtual_sensors.clear();
	VirtualSensor s;
	Vector3f axis;
	float angle;
	char c;
	char use;
	while ( inpFile >> use) {
		inpFile >> s.bonename;
		inpFile >> s.relative_position.x >> s.relative_position.y
				>> s.relative_position.z;

		inpFile >> c;
		if (c=='q') {
			inpFile >> s.relative_orientation.x >> s.relative_orientation.y
					>> s.relative_orientation.z >> s.relative_orientation.w;
		} else if (c=='a') {
			inpFile >> axis.x >> axis.y >> axis.z >> angle;
			s.relative_orientation = normalize(rotation(angle, axis));
		} else
			cout << "Invalid sensor for bone " << s.bonename << endl;

		inpFile >> c;
		if (c != 't')
			cout << c << ": No transform...confused parsing " << s.bonename << endl;

		inpFile >> s.asf2v.x >> s.asf2v.y >> s.asf2v.z >> s.asf2v.w;

		cout << "Got asf2v " << s.asf2v << endl;
		if (use == '_')
			virtual_sensors.push_back(s);
		else if (use == '%') { }
		else cout << "parse error in sensor file" << endl;
	}
	inpFile.close();

}

void apply_virtual_sensors_only_one_motion(int motion) {
	vector< vector< Vector3f > > wb_bases;
	vector< vector< Quatf > > wb_orientations;

	slurp_motion_data(motion, wb_bases, wb_orientations);

	set_accelerations(motion, false, wb_bases, wb_orientations);
}

void apply_virtual_sensors() {
	// this is important for code assumptions!!!
	//assert(virtual_sensors.size() == NUMSENSORS);

	// for each motion, set its accelerations array
	for (unsigned int mm = 0; mm < Library::motion_count(); mm++) {
		Library::Motion & m = Library::motion_nonconst(mm);
		//m.accelerations.clear();
		if (m.frames() < 10) continue; // not worth the bother, really

		bool is_v;
		string f = m.filename;
		if (f[f.size() - 1] == 'V' || f[f.size() - 1] == 'v')
			is_v = true;
		else if (f[f.size() - 1] == 'C' || f[f.size() - 1] == 'c')
			is_v = false;
		else {
			cout << "Skeleton filename: " << f << endl;
			assert(0);
		}
		is_v = false;



		vector< vector< Vector3f > > wb_bases;
		vector< vector< Quatf > > wb_orientations;

		slurp_motion_data(mm, wb_bases, wb_orientations);
		
		//if (! is_v ) smooth_data(wb_bases, wb_orientations);

		set_accelerations(mm, is_v, wb_bases, wb_orientations);

	}
}

void slurp_motion_data(int motion, vector< vector< Vector3f > > &wb_bases, vector< vector< Quatf > > &wb_orientations) {
	const Library::Motion &m = Library::motion(motion);
	vector< vector< Vector3f > > bases(m.frames());
	vector< vector< Quatf > > orientations(m.frames());
	for (unsigned int f = 0; f < m.frames(); f++) {
		Character::Pose p;
		m.get_pose(f, p);
		Character::WorldBones wb;
		get_world_bones(p, wb);
		bases[f] = wb.bases;
		orientations[f] = wb.orientations;
	}
	wb_bases = bases;
	wb_orientations = orientations;
}


void smooth_data(vector< vector< Vector3f > > &wb_bases, vector< vector< Quatf > > &wb_orientations) {
	vector< vector< Vector3f > > wb_bases_t = wb_bases;
	vector< vector< Quatf > > wb_orientations_t = wb_orientations;
	int k = 10;
	for (int b = 0; b < (int)wb_bases[0].size(); b++)
		for (int f = k; f < int(wb_bases.size()) - k; f++) {
			// this is a median filter

			// this is a mean filter
			for (int kk = -k; kk <= k; kk++) {
				wb_bases_t[f][b] += wb_bases[f+kk][b];
				wb_orientations_t[f][b] = normalize(wb_orientations_t[f][b]) + abs(wb_orientations[f+kk][b]);
			}
			wb_bases_t[f][b] *= 1.0/(2 * k + 1);
			wb_orientations_t[f][b] = normalize(wb_orientations_t[f][b]);

			// this is a zero filter
			//wb_bases_t[f][b] = make_vector< float, 3 >(0.0f); // DEBUGGG!
			//wb_orientations_t[f][b].clear(); // DEBUGGGG!
		}

	wb_bases = wb_bases_t;
	wb_orientations = wb_orientations_t;
}


void set_accelerations(int motion, bool is_v, const vector< vector< Vector3f > > &wb_bases, const vector< vector< Quatf > > &wb_orientations) {
	Library::Motion &m = Library::motion_nonconst(motion);

	vector< Vector3f > temp_p(virtual_sensors.size());
	vector< Quatf > temp_r(virtual_sensors.size());
	vector< vector< Vector3f > > sensor_positions(m.frames(), temp_p);
	vector< vector< Quatf    > > sensor_rotations(m.frames(), temp_r);

	for (int s = 0; s < (int)virtual_sensors.size(); s++) {
		int b = m.skeleton->get_bone_by_name(virtual_sensors[s].bonename);
		assert(b != -1);
		for (unsigned int f = 0; f < m.frames(); f++) {
			Quatf wborient = is_v ? wb_orientations[f][b] : multiply(wb_orientations[f][b], Character::virtual_sensors[s].asf2v);
			//Quatf wborient = wb_orientations[f][b];
			//cout << "Multiplying sensor by " << Character::virtual_sensors[s].asf2v << endl;

			//cout << "wb_bases[f][b] = " << wb_bases[f][b] << endl;
			//cout << "wborient = " << wborient << endl;
			//cout << "virtual_sensors[s].relative_position = " << virtual_sensors[s].relative_position << endl;
			sensor_positions[f][s] = wb_bases[f][b] +
				rotate(virtual_sensors[s].relative_position, wborient);
			sensor_rotations[f][s] = multiply(wborient, virtual_sensors[s].relative_orientation);
			//cout << "Setting position = " << sensor_positions[f][s] << endl;
		}
	}


	// compute accelerations
	vector< vector< float > > accelerations(m.frames());
	vector< float > temp(virtual_sensors.size() * 3, 0.0f);
	accelerations[0] = temp;
	accelerations[m.frames() - 1] = temp;

	for (unsigned int f = 1; f < m.frames() - 1; f++) {
		vector< float > acc_float(virtual_sensors.size() * 3);
		for (int s = 0; s < (int)virtual_sensors.size(); s++) {
			
			// calculate global-frame acceleration
			Vector3f acc;
			acc = sensor_positions[f+1][s]
						- 2*sensor_positions[f][s] + sensor_positions[f-1][s];
			acc *= FPS * FPS;
				
			// subtract gravity
			acc.y += 9.8; // dists in .V files are in meters
				
			// convert to bone-frame (local) acceleration
			acc = rotate(acc, normalize(conjugate(sensor_rotations[f][s])));

			acc /= 9.8; // normalize to g

			// concept of pos/neg is reversed in actual accelerometers
			acc_float[s * 3    ] = -acc.x;
			acc_float[s * 3 + 1] = -acc.y;
			acc_float[s * 3 + 2] = - (-1 * acc.z);
			//acc_float[s * 3    ] = -acc.x;
			//acc_float[s * 3 + 1] = -acc.y;
			//acc_float[s * 3 + 2] = -acc.z;
			//if (f % 200 == 0 && s==1) cout << "acc = " << acc << endl;

		}
		accelerations[f] = acc_float;
	}
	/*for (unsigned int f = 1; f < m.frames() - 1; f++)
		for (int s = 0; s < (int)virtual_sensors.size(); s++) {
			if (accelerations[f][s] < -3) accelerations[f][s] = -3;
			if (accelerations[f][s] >  3) accelerations[f][s] =  3;
			accelerations[f][s] *= .9;
		}
	*/

	//if (! is_v ) smooth_accelerations(accelerations);
	//smooth_accelerations(accelerations);
	m.accelerations = accelerations;
	//cout << "Accelerations: " << endl;
	//for (unsigned int f = 0; f < m.frames(); f++)
	//	cout << accelerations[f][1] << " ";
}



float min(float a, float b) {
	return a < b ? a : b;
}
float max(float a, float b) {
	return a > b ? a : b;
}
void smooth_accelerations(vector< vector< float > > &acc) {
	// median filter
	for (int s = 0; s < (int)virtual_sensors.size(); s++) {
		for (int f = 2; f < (int)acc.size() - 2; f++) {
			float a = acc[f-1][s];
			float b = acc[f][s];
			float c = acc[f+1][s];
			acc[f][s] = a + b + c - min(a, min(b,c)) - max(a,max(b,c));
		}
	}
}

void save_virtual_sensors(string filename) {
	cout << "Saving virtual sensors to file " << filename << "." << endl;
	ofstream outFile(filename.c_str());
	assert(outFile);
	for (unsigned int s = 0; s < virtual_sensors.size(); s++) {
		outFile << "_ ";
		outFile << virtual_sensors[s].bonename << " ";
		outFile << virtual_sensors[s].relative_position.x << " ";
		outFile << virtual_sensors[s].relative_position.y << " ";
		outFile << virtual_sensors[s].relative_position.z << " ";
		outFile << "q ";
		outFile << virtual_sensors[s].relative_orientation.x << " ";
		outFile << virtual_sensors[s].relative_orientation.y << " ";
		outFile << virtual_sensors[s].relative_orientation.z << " ";
		outFile << virtual_sensors[s].relative_orientation.w << " ";
		outFile << "t ";
		outFile << virtual_sensors[s].asf2v.x << " ";
		outFile << virtual_sensors[s].asf2v.y << " ";
		outFile << virtual_sensors[s].asf2v.z << " ";
		outFile << virtual_sensors[s].asf2v.w << " ";
		outFile << endl;
	}
	outFile.close();
	cout << "Done saving virtual sensors." << endl;
}

} // end namespace
