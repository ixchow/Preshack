#ifndef SS_OPTIMIZER
#define SS_OPTIMIZER

#include <Vector/Vector.hpp>

#include <vector>
#include <string>

#include <stdint.h>

using std::vector;
using std::string;

class Layer {
public:
	vector< uint32_t > data;
	Vector4f col(uint32_t at) {
		return make_vector(
			(data[at] & 0xff) / 255.0f,
			((data[at] >> 8) & 0xff) / 255.0f,
			((data[at] >> 16) & 0xff) / 255.0f,
			(data[at] >> 24) / 255.0f
		);
	}
	//permeability. stored in alternating rows of
	// - (size.x-1) (perm in x direction)
	// - (size.x) (perm in y direction
	// ...
	// last row is a x-direction row, as you'd expect.
	vector< float > perm;
	void perm_from_alpha(Vector2ui size);
	void perm_from_color(Vector2ui size);
};

class Constraint {
public:
	vector< uint32_t > valid_orders;
	vector< uint32_t > data;
};

class Cons {
public:
	float weight;
	unsigned int layers_count; //this is redundant but saves me from a lot of refactoring.
	vector< uint32_t > valid_orders;
};

class SolverData;

class Problem {
public:
	void downsample_from(Problem &other, SolverData *);
	Vector2ui size;
	vector< Layer > layers;
	vector< Constraint > constraints;
/*
	vector< float > alphas_at(size_t ofs) {
		assert(ofs < size.x * size.y);
		vector< float > ret(layers.size());
		for (unsigned int l = 0; l < layers.size(); ++l) {
			ret[l] = (layers[l].data[ofs] >> 24) / 255.0f;
		}
		return ret;
	}

	vector< float > alphas_at(Vector2ui p) {
		assert(p.x < size.x);
		assert(p.y < size.y);
		return alphas_at(p.x + p.y * size.x);
	}

	vector< float > alphas_between(Vector2ui p, Vector2ui n) {
		vector< float > a = alphas_at(p);
		vector< float > b = alphas_at(n);
		for (unsigned int i = 0; i < a.size(); ++i) {
			a[i] = (a[i] + b[i]) * 0.5;
		}
		return a;
	}
	*/
	vector< float > alphas_between(Vector2ui p, Vector2ui n) {
		unsigned int ofs = 0;
		if (p.x != n.x) {
			assert(p.y == n.y);
			if (p.x > n.x) std::swap(p.x, n.x);
			assert(p.x + 1 == n.x);
			assert(p.x + 1 < size.x);
			assert(p.y < size.y);
			ofs = p.y * (size.x-1+size.x) + p.x;
		} else if (p.y != n.y) {
			assert(p.x == n.x);
			if (p.y > n.y) std::swap(p.y, n.y);
			assert(p.y + 1 == n.y);
			assert(p.x < size.x);
			assert(p.y + 1 < size.y);
			ofs = p.y * (size.x-1+size.x) + (size.x-1) + p.x;
		}
		vector< float > ret(layers.size());
		for (unsigned int i = 0; i < ret.size(); ++i) {
			assert(ofs < layers[i].perm.size());
			ret[i] = layers[i].perm[ofs];
		}
		return ret;
	}

	vector< uint32_t > layers_at(size_t ofs) {
		assert(ofs < size.x * size.y);
		vector< uint32_t > ret;
		for (unsigned int l = 0; l < layers.size(); ++l) {
			if (layers[l].data[ofs] & 0xff000000) {
				ret.push_back(l);
			}
		}
		return ret;
	}

	vector< uint32_t > layers_at(Vector2ui p) {
		assert(p.x < size.x);
		assert(p.y < size.y);
		return layers_at(p.x + p.y * size.x);
	}

	vector< Cons > constraints_at(size_t ofs) {
		assert(ofs < size.x * size.y);
		vector< Cons > ret;
		for (unsigned int c = 0; c < constraints.size(); ++c) {
			if (constraints[c].data[ofs] & 0xff000000) {
				ret.push_back(Cons());
				ret.back().weight = (constraints[c].data[ofs] >> 24) / 255.0f;
				ret.back().valid_orders = constraints[c].valid_orders;
				ret.back().layers_count = layers.size();
			}
		}
		return ret;
	}

	vector< Cons > constraints_at(Vector2ui p) {
		assert(p.x < size.x);
		assert(p.y < size.y);
		return constraints_at(p.x + p.y * size.x);
	}
};

class SolveStatus {
public:
	virtual ~SolveStatus() { }
	virtual bool is_running() = 0;
	virtual void get_current_solution(vector< uint32_t > &into, Vector2ui &size, vector< vector< uint32_t > > *coefs = NULL) = 0;
	virtual bool get_new_solution(vector< uint32_t > &into, Vector2ui &size, uint32_t &serial, vector< vector< uint32_t > > *coefs = NULL) = 0; //returns 'true' if serial changed.
};

SolveStatus *start_problem(Problem *p, bool save_coefs);
//Problem is now owned by solver. You should forget about it immediately.
//Returned SolveStatus can be deleted to stop running the problem.

#endif //SS_OPTIMIZER
