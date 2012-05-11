#include "Item.hpp"
#include <Graphics/Graphics.hpp>

void glBoxToBox(Box2f const &internal, Box2f const &external) {
	glTranslatef(external.min.x, external.min.y, 0.0f);
	glScalef(
		(external.max.x - external.min.x)/(internal.max.x - internal.min.x),
		(external.max.y - external.min.y)/(internal.max.y - internal.min.y),
		1.0f);
	glTranslatef(-internal.min.x, -internal.min.y, 0.0f);
}

bool Item::get_pixel_size(Vector2ui &into) {
	into = make_vector(0U, 0U);
	return false;
}
