#pragma once

#include "Matrix-loose.hpp"

inline void glMultMatrix(Transform2d const &xform) {
	GLdouble mat[16] = {
		GLdouble(xform(0,0)), GLdouble(xform(1,0)), 0.0, 0.0,
		GLdouble(xform(0,1)), GLdouble(xform(1,1)), 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		GLdouble(xform(0,2)), GLdouble(xform(1,2)), 0.0, 1.0,
	};
	glMultMatrixd(mat);
}

inline void glMultMatrix(Transform2f const &xform) {
	GLfloat mat[16] = {
		GLfloat(xform(0,0)), GLfloat(xform(1,0)), 0.0f, 0.0f,
		GLfloat(xform(0,1)), GLfloat(xform(1,1)), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		GLfloat(xform(0,2)), GLfloat(xform(1,2)), 0.0f, 1.0f,
	};
	glMultMatrixf(mat);
}
