#ifndef SHAPES_HPP
#define SHAPES_HPP

#include <cmath>

template< int angles >
Vector2f const *angles_table() {
	static Vector2f table[angles + 1]; //angles table is circular for ease-of-use reasons.
	static bool init = false;
	if (!init) {
		init = true;
		for (unsigned int a = 0; a <= angles; ++a) {
			float ang = a / float(angles) * float(M_PI) * 2.0f;
			table[a] = make_vector(cosf(ang), sinf(ang));
		}
	}
	return table;
}

template< int angles >
void circle( float radius = 1.0f, Vector2f center = make_vector(0.0f, 0.0f)) {
	glPushAttrib(GL_TRANSFORM_BIT);
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(center.x, center.y, 0.0f);
	glScalef(radius, radius, 1.0f);

	Vector2f const *tab = angles_table< angles >();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &(tab[0]));
	glDrawArrays(GL_TRIANGLE_FAN, 0, angles);

	glPopMatrix();
	glPopClientAttrib();
	glPopAttrib();
}

/*template< int angles >
void capsule( float length = 1.0f, float Vector2f center = make_vector(0.0f, 0.0f), float length = 1.0f, float r1*/


#endif //SHAPES_HPP
