#ifndef SS_CONSTRAINT_HPP
#define SS_CONSTRAINT_HPP

#include <Graphics/Graphics.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include "Module.hpp"

#include <vector>


const Vector2f LayerBoxSize = make_vector(0.2f, 0.18f);
const Vector2f AlphaBoxSize = make_vector(1.0f, 0.06f);

class LayerBox {
public:
	LayerBox() : layer((unsigned)-1), pos(make_vector(0.0f, 0.0f)), target(make_vector(0.0f, 0.0f)), color(make_vector(1.0f, 0.0f, 1.0f)) { }
	unsigned int layer;
	Vector2f pos;
	Vector2f target;
	Vector3f color;
};

class LayerBin {
public:
	LayerBin() : col(0) { }
	std::vector< LayerBox * > cont;
	unsigned int col;
	Box2f get_box();
};

class StackingConstraint : public Module {
public:
	StackingConstraint();
	virtual ~StackingConstraint();
	virtual Vector2f size();
	virtual bool handle_event(SDL_Event const &e, Vector2f mouse);
	virtual void update(float elapsed);
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	
	std::vector< LayerBox > boxes;
	std::vector< LayerBin > bins;
	LayerBox *current_box;
	unsigned int box_target_seam;
	unsigned int box_target_col;
	bool dragging_box;
	void layout_boxes();

	void make_closest_wanted();

	PortDataContainer< std::vector< Vector3f > > layer_colors;
	PortDataContainer< std::vector< uint32_t > > closest_wanted;

	Vector2f cur_mouse;
};


#endif //SS_CONSTRAINT_HPP
