#ifndef ITEM_HPP
#define ITEM_HPP

#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include <cstdlib>

typedef unsigned int ItemId;

class Item {
public:
	Item() : id(rand()), position(make_vector(0.0f, 0.0f)), scale(1.0f) {
	}
	virtual ~Item() {}
	ItemId id;
	Vector2f position; //center position
	virtual Vector2f size() = 0; //size ['natural']
	float scale; //scale of natural size.
	//viewport is the area of you that is seen.
	//screen_viewport is where this maps on the screen.
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0) = 0;
	//returns 'true' and sets 'into' if item has a preferred size in pixels:
	virtual bool get_pixel_size(Vector2ui &into);
};

void glBoxToBox(Box2f const &internal, Box2f const &external);

#endif //ITEM_HPP
