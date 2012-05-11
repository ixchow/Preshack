#ifndef OBJ_HPP
#define OBJ_HPP

#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>
#include <string>
#include <vector>

using std::string;
using std::vector;

class ObjModel;

bool load_obj(string filename, ObjModel &into);

class ObjModel {
public:
	void clear() {
		verts.clear();
		quads.clear();
		tris.clear();
	}
	vector< Vector3f > verts;
	vector< Vector< unsigned int, 4 > > quads;
	vector< Vector< unsigned int, 3 > > tris;
	Box3f bounds;
	void emit_as_quads(); //emit gl verts, making tris into quads.
};

#endif //OBJ_HPP
