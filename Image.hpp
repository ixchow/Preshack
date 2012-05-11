#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "Item.hpp"

#include <Graphics/Texture.hpp>

#include <string>

using std::string;

class Image : public Item {
public:
	Image() : path("") {
		bad_path = path;
	}
	virtual ~Image() {}
	string path;
	string bad_path;
	Graphics::TextureRef get_texture();
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual bool get_pixel_size(Vector2ui &into);
};

#endif //IMAGE_HPP
