#ifndef MATRIX_LOOSE_HPP
#define MATRIX_LOOSE_HPP

#include "Vector.hpp"
#include "Matrix.hpp"

//Define some slightly sloppy syntax which -- in practice -- makes
//transformation matrices much easier to work with.

typedef Matrix< double, 2, 3 > Matrix2x3d;
typedef Matrix< float, 2, 3 > Matrix2x3f;

typedef Matrix2x3d Transform2d;
typedef Matrix2x3f Transform2f;



template< typename NUM >
bool invert(Matrix< NUM, 2, 3 > const &trans, Matrix< NUM, 2, 3 > &into) {
	NUM a = trans(0,0);
	NUM b = trans(0,1);
	NUM c = trans(1,0);
	NUM d = trans(1,1);

	NUM det = a * d - c * b;
	if (det == 0.0) {
		into = identity_matrix< NUM, 2, 3 >();
		return false;
	}
	into(0,0) =  d / det;
	into(0,1) = -b / det;
	into(1,0) = -c / det;
	into(1,1) =  a / det;

	NUM x = trans(0,2);
	NUM y = trans(1,2);

	//pt = M * src + (x,y)
	//pti = M^{-1} * M * src + M*{-1} * (x,y)
	into(0,2) = -(into(0,0) * x + into(0,1) * y);
	into(1,2) = -(into(1,0) * x + into(1,1) * y);

	return true;
}

template< typename NUM >
Matrix< NUM, 2, 3 > invert(Matrix< NUM, 2, 3 > const &mat) {
	Matrix< NUM, 2, 3 > ret;
	if (!invert(mat, ret)) {
		ret(0,0) = 0.0;
		ret(0,1) = 0.0;
		ret(0,2) = -mat(0,2);
		ret(1,0) = 0.0;
		ret(1,1) = 0.0;
		ret(1,2) = -mat(1,2);
	}
	return ret;
}



template< typename NUM >
Matrix< NUM, 2, 3 > operator*(Matrix< NUM, 2, 3 > const &a, Matrix< NUM, 2, 3 > const &b) {
	return a * make_matrix(b, transpose(make_matrix(make_vector< NUM >(0.0, 0.0, 1.0))));
}

template< typename NUM >
Vector< NUM, 2 > operator*(Matrix< NUM, 2, 3 > const &m, Vector< NUM, 2 > const &v) {
	return make_vector(
		m(0,0) * v[0] + m(0,1) * v[1] + m(0,2),
		m(1,0) * v[0] + m(1,1) * v[1] + m(1,2)
		);
}

#endif //MATRIX_LOOSE_HPP
