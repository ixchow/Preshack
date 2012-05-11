#include "LL_Obj.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <fstream>
#include <sstream>
#include <cassert>

using std::ifstream;
using std::istringstream;
using std::getline;

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl


bool load_obj(string filename, ObjModel &into) {
	ifstream file(filename.c_str());
	string line;
	while (getline(file, line)) {
		istringstream line_str(line);
		string tok;
		if (!(line_str >> tok) || !tok.size() || tok[0] == '#') continue;
		if (tok == "usemtl" || tok == "s") {
			//ignore....
		} else if (tok == "v") {
			Vector3f vert;
			if (!(line_str >> vert.x >> vert.y >> vert.z)) {
				LOG_WARNING("Failed to read vertex from line '" << line << "' in file '" << filename << "'");
				into.verts.push_back(make_vector(0.0f, 0.0f, 0.0f));
			} else {
				into.verts.push_back(vert);
			}
		} else if (tok == "f") {
			vector< int > coords;
			int temp = 0;
			while (line_str >> temp) {
				if (temp < 1) {
					LOG_WARNING("Crazy last-n-vertex indexing... will ignore.");
				} else {
					coords.push_back(temp-1);
				}
			}
			if (coords.size() == 4) {
				into.quads.push_back(make_vector((unsigned)coords[0], (unsigned)coords[1], (unsigned)coords[2], (unsigned)coords[3]));
			} else if (coords.size() == 3) {
				into.tris.push_back(make_vector((unsigned)coords[0], (unsigned)coords[1], (unsigned)coords[2]));
			} else {
				LOG_WARNING("Ignoring odd-sized (" << coords.size() << ") face.");
			}
		} else {
			LOG_WARNING("Ignoring unknown token '" << tok << "' in file '" << filename << "'");
		}
	}
	for (unsigned int v = 0; v < into.verts.size(); ++v) {
		if (v == 0) {
			into.bounds.min = into.bounds.max = into.verts[v];
		}
		into.bounds.min = min(into.bounds.min, into.verts[v]);
		into.bounds.max = max(into.bounds.max, into.verts[v]);
	}
	return into.quads.size() || into.tris.size();
}

void ObjModel::emit_as_quads() {
	//Lazy lazy lazy....
	for (unsigned int q = 0; q < quads.size(); ++q) {
		for (unsigned int i = 0; i < 4; ++i) {
			assert(quads[q].c[i] < verts.size());
		}
		glNormal(normalize(cross_product(
			verts[quads[q].c[1]] - verts[quads[q].c[0]],
			verts[quads[q].c[2]] - verts[quads[q].c[0]])));
		for (unsigned int i = 0; i < 4; ++i) {
			glVertex(verts[quads[q].c[i]]);
		}
	}
	for (unsigned int t = 0; t < tris.size(); ++t) {
		//check:
		for (unsigned int i = 0; i < 3; ++i) {
			assert(tris[t].c[i] < verts.size());
		}
		glNormal(normalize(cross_product(
			verts[tris[t].c[1]] - verts[tris[t].c[0]],
			verts[tris[t].c[2]] - verts[tris[t].c[0]])));
		for (unsigned int i = 0; i < 3; ++i) {
			glVertex(verts[tris[t].c[i]]);
		}
		glVertex(verts[tris[t].c[2]]); //double-up on last vert.
	}
}
