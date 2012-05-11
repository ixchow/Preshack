#ifndef VECTOR_GL_HPP
#define VECTOR_GL_HPP 1

//include your favorite opengl header first.


//generate prototypes for glFN3T( Vector< TYPE, {2,3} > ):
//(broken out because of glNormal)
#define glFN3( FN, TYPE, T ) \
	inline void gl ## FN(Vector< TYPE, 3 > const &v) { \
		gl ## FN ## 3 ## T ## v(v.c); \
	} \
	inline void gl ## FN(Vector< TYPE, 2 > const &v, TYPE const &z) { \
		gl ## FN ## 3 ## T (v.c[0], v.c[1], z); \
	} \

//generate prototypes for glFN4T( Vector< TYPE, {3,4} > ):
//(broken out because of glColor)
#define glFN4( FN, TYPE, T ) \
	inline void gl ## FN(Vector< TYPE, 4 > const &v) { \
		gl ## FN ## 4 ## T ## v(v.c); \
	} \
	inline void gl ## FN(Vector< TYPE, 3 > const &v, TYPE const &w) { \
		gl ## FN ## 4 ## T (v.c[0], v.c[1], v.c[2], w); \
	} \



//generate prototypes for glFN{2,3,4}T( Vector< TYPE, {2,3,4} > ):
#define glFN234( FN, TYPE, T ) \
	inline void gl ## FN (Vector< TYPE, 2 > const &v) { \
		gl ## FN ## 2 ## T ## v(v.c); \
	} \
	glFN3( FN, TYPE, T ) \
	glFN4( FN, TYPE, T ) \


#define DO( TYPE, T ) \
	glFN234( Vertex, TYPE, T ) \
	glFN3( Normal, TYPE, T ) \
	glFN3( Color, TYPE, T ) \
	glFN4( Color, TYPE, T ) \
	glFN234( TexCoord, TYPE, T ) \

DO( GLint, i );
DO( GLfloat, f );
DO( GLdouble, d );


glFN3( Color, GLubyte, ub );
glFN4( Color, GLubyte, ub );

#undef DO
#undef glFN234
#undef glFN4
#undef glFN3


inline void glTranslate(Vector< float, 2 > const &v, float z = 0) {
	glTranslatef(v.c[0], v.c[1], z);
}

inline void glTranslate(Vector< float, 3 > const &v) {
	glTranslatef(v.c[0], v.c[1], v.c[2]);
}

inline void glTranslate(Vector< double, 2 > const &v, double z = 0) {
	glTranslated(v.c[0], v.c[1], z);
}

inline void glTranslate(Vector< double, 3 > const &v) {
	glTranslated(v.c[0], v.c[1], v.c[2]);
}


#endif
