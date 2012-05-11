#ifndef COEFS_HPP
#define COEFS_HPP

#include <stdint.h>

#include <vector>
#include <algorithm>
#include <cassert>

typedef uint32_t LayerIndex;

typedef uint32_t StackingIndex;
typedef std::vector< LayerIndex > Stacking;

typedef uint32_t TwoSetIndex;
typedef std::pair< LayerIndex, LayerIndex > TwoSet;

typedef uint32_t DeltaIndex;
class Delta {
public:
	Delta() : pos(-1U) {
	}
	Delta(Stacking const &_base, unsigned int _pos) : base(_base), pos(_pos) {
		assert(pos + 1 < base.size() && pos < base.size());
		if (base[pos] > base[pos+1]) {
			std::swap(base[pos], base[pos+1]);
		}
	}
	Stacking base; //stacking with flip (in positive order)
	unsigned int pos; //position of the flip.
};

StackingIndex to_stacking_index(Stacking const &in);
Stacking to_stacking(StackingIndex index, unsigned int layers);
size_t count_stackings(unsigned int layers);

DeltaIndex to_delta_index(Delta const &in);
Delta to_delta(DeltaIndex index, unsigned int layers);
size_t count_deltas(unsigned int layers);

TwoSet to_twoset(TwoSetIndex index);
TwoSetIndex to_twoset_index(TwoSet const &in);
TwoSetIndex count_twosets(unsigned int layers);

//helper call to lapack.
void pinv_and_null(const float *matrix, unsigned int rows, unsigned int cols, std::vector< float > &pinv, std::vector< float > &null);


//returns a matrix of partials, count_stackings(alphas.size())^2 in size,
//stored in row-major order, with row i, column j representing the coef
//of stacking difference j in the partial of the objective function w.r.t. the
//i'th stacking difference
void build_factors_matrix(std::vector< float > const &alphas, std::vector< float > &into);


void test_indexing(); //run internal tests.

void test_svd(); //test svd ops.

#endif //COEFS_HPP
