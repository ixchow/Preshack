#ifndef EQUATION_HPP
#define EQUATION_HPP

#include "Item.hpp"

#include <Graphics/Texture.hpp>

#include <string>

using std::string;

class Equation : public Item {
public:
	Equation(string const &_equation = "") : equation(_equation) {
		bad_equation = "";
	}
	virtual ~Equation() {}
	string equation;
	private:
	string bad_equation;
	Graphics::TextureRef get_texture();
	public:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	virtual bool get_pixel_size(Vector2ui &into);

};

#endif //EQUATION_HPP
