#ifndef STRING_HPP
#define STRING_HPP

#include "Item.hpp"

#include <string>

using std::string;

class String : public Item {
public:
	String() : text("nothing"), color(make_vector(1.0f, 1.0f, 1.0f)) {
	}
	virtual ~String() {}
	string text;
	Vector3f color;
	virtual Vector2f size() override;
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0) override;
	virtual bool get_pixel_size(Vector2ui &into) override;

};

#endif //STRING_HPP
