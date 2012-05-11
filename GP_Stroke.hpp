#ifndef STROKE_HPP
#define STROKE_HPP

//Convenient container for keeping track of a user stroke.
//Idea:
//  points get submitted ->
//  stroke chopped up into short segments ->
//  short segments smoothed ->
//  final stroke geometry generated (incrementally based on smoothed segments).


#include <Vector/Vector.hpp>

#include <vector>
#include <deque>

using std::vector;
using std::deque;

class Stroke {
public:
	Stroke() {
		clear();
	}
	virtual ~Stroke() { }
	virtual void clear();
	virtual void submit_point(Vector2f const &new_point);
	vector< Vector2f > points;
	unsigned int new_points; //basically a 'dirty' flag, for incremental computation.
	bool dirty; //really a dirty flag (i.e. "re-do the entire thing")
};

class SmoothStroke : public Stroke {
public:
	SmoothStroke() {
		Step = 1;
		smoothing_kernel.push_back( 0.25f );
		smoothing_kernel.push_back( 0.50f );
		smoothing_kernel.push_back( 0.25f );
		clear();
	}
	virtual ~SmoothStroke() { }
	virtual void clear();
	virtual void submit_point(Vector2f const &new_point);
//options:
	vector< float > smoothing_kernel;
	float Step;
//dynamic stuff:
	Vector2f last;
	float along;
	deque< Vector2f > smoothing_queue;
};


#endif //STROKE_HPP
