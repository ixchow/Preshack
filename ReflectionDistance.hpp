#ifndef REFLECTION_DISTANCE_HPP
#define REFLECTION_DISTANCE_HPP

#include "GameStuff.hpp"

#include <Vector/Vector.hpp>

#include <vector>
#include <string>

using std::vector;
using std::string;

class ReflectionDistance {
public:
	static string const &name();
	ReflectionDistance(Game2d::Fragment const &from);
	float distance(Game2d::Fragment const &to);
	vector< vector< Vector3f > > tips;
};


#endif //REFLECTION_DISTANCE_HPP
