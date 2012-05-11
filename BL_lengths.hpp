#ifndef BL_CSV_HPP
#define BL_CSV_HPP

#include "Module.hpp"

using std::vector;
using std::string;

class Lengths : public Module {
public:
	Lengths();
	virtual ~Lengths();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual float cursor_alpha();
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f mouse);

	vector< double > mins;
	vector< double > maxes;
	vector< unsigned int > order;
	unsigned int prev_frame;

	bool have_cursor;
	float cursor_x;

	vector< double > splits; //for nonlinear scaling

	PortDataContainer< vector< double > > lengths;
	PortDataContainer< unsigned int > frame;
	PortDataContainer< vector< unsigned int > > highlight;
};

#endif //BL_CSV_HPP
