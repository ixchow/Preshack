#ifndef BL_CSV_HPP
#define BL_CSV_HPP

#include "Module.hpp"

using std::vector;
using std::string;

class CSVMotion : public Module {
public:
	CSVMotion(string filename, double seconds_per_frame);
	virtual ~CSVMotion();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	vector< string > names;
	vector< vector< Vector3d > > positions;
	vector< Vector3d > roots; //(x,y, z_rot)
	Box3d bounds; //bounds of local poses

	bool playing;
	double time_acc;
	double seconds_per_frame;

	PortDataContainer< vector< double > > lengths;
	PortDataContainer< unsigned int > frame;
	PortDataContainer< vector< unsigned int > > highlight;

	void update_lengths();
};

#endif //BL_CSV_HPP
