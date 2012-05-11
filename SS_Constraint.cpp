#include "SS_Constraint.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>
#include <Basis/Error.hpp>
#include <Graphics/load_save_png.hpp>

#include <vector>

#include "SS_coefs.hpp"

using std::vector;
using std::make_pair;

namespace {
	Module *create_module(const std::string &params_in) {
		return new StackingConstraint();
	}
	class Fred {
	public:
		Fred() {
			register_module("ss_constraint", create_module, "");
		}
	} fred;
}


Box2f LayerBin::get_box() {
	assert(cont.size());
	Box2f ret;
	ret.min = ret.max = cont[0]->target;
	for (unsigned int i = 1; i < cont.size(); ++i) {
		ret.min = min(ret.min, cont[i]->target);
		ret.max = max(ret.max, cont[i]->target);
	}
	ret.min -= 0.5f * LayerBoxSize;
	ret.max += 0.5f * LayerBoxSize;
	return ret;
}

namespace {
	void draw_nothing(const PortData *, const Connection &, void *) {
	}
}

StackingConstraint::StackingConstraint() {
	current_box = NULL;
	box_target_seam = -1U;
	box_target_col = -1U;
	dragging_box = false;

	cur_mouse = make_vector(0.0f, 0.0f);

	layer_colors.name = "colors";
	layer_colors.position = make_vector(0.0f, 0.0f);
	layer_colors.draw_fn = draw_nothing;

	closest_wanted.name = "cons";
	closest_wanted.position = make_vector(-0.1f, 0.0f);
	closest_wanted.draw_fn = draw_nothing;

	out_ports.push_back(&closest_wanted);
	in_ports.push_back(&layer_colors);

}

StackingConstraint::~StackingConstraint() {
}

bool StackingConstraint::handle_event(SDL_Event const &e, Vector2f mouse) {
	if (e.type == SDL_MOUSEMOTION) {
		cur_mouse = mouse;
		if (dragging_box) {
			assert(current_box);
			current_box->pos = cur_mouse;
			//remove current_box from whatever bin it is in at the moment:
			for (unsigned int b = 0; b < bins.size(); ++b) {
				for (unsigned int c = 0; c < bins[b].cont.size(); ++c) {
					if (bins[b].cont[c] == current_box) {
						assert(box_target_seam == -1U);
						assert(box_target_col == -1U);
						bins[b].cont.erase(bins[b].cont.begin() + c);
						break;
					}
				}
			}
			layout_boxes(); //in case columns decide to change.
			//figure out where it should actually be:
			unsigned int close_bin = -1U;
			float close_dis = 2.0f;
			for (unsigned int b = 0; b < bins.size(); ++b) {
				Box2f box = bins[b].get_box();
				Vector2f in = box.clamp(cur_mouse);
				float dis = length_squared(in - cur_mouse);
				if (dis < close_dis || close_bin >= bins.size()) {
					close_dis = dis;
					close_bin = b;
				}
			}
			if (close_bin < bins.size()) {
				Box2f box = bins[close_bin].get_box();
				Vector2f ofs = current_box->pos - box.center();
				if (ofs.x > LayerBoxSize.x * 0.3f) {
					box_target_col = bins[close_bin].col;
					box_target_seam = -1U;
				} else if (ofs.x <-LayerBoxSize.x * 0.3f) {
					box_target_col = bins[close_bin].col+1;
					box_target_seam = -1U;
				} else if (ofs.y > LayerBoxSize.y * 0.3f) {
					box_target_seam = close_bin+1;
					box_target_col = bins[close_bin].col;
				} else if (ofs.y <-LayerBoxSize.y * 0.3f) {
					box_target_seam = close_bin;
					box_target_col = bins[close_bin].col;
				} else {
					bins[close_bin].cont.push_back(current_box);
					box_target_seam = -1U;
					box_target_col = -1U;
				}
			} else {
				box_target_seam = 0;
				box_target_col = 0;
			}
			//And recompute:
			layout_boxes();
			return true;
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		if (current_box) {
			for (unsigned int b = 0; b < bins.size(); ++b) {
				for (unsigned int c = 0; c < bins[b].cont.size(); ++c) {
					if (bins[b].cont[c] == current_box) {
						dragging_box = true;
						box_target_seam = b;
						box_target_col = bins[b].col;
						bins[b].cont.erase(bins[b].cont.begin() + c);
						if (bins[b].cont.empty()) {
							bins.erase(bins.begin() + b);
						}
						layout_boxes();
						break;
					}
				}
				if (dragging_box) break;
			}
			return true;
		}
	}
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		if (dragging_box) {
			assert(current_box);
			current_box->pos = cur_mouse;
			if (box_target_seam <= bins.size()) {
				assert(box_target_col != -1U);
				bins.insert(bins.begin() + box_target_seam, LayerBin());
				bins[box_target_seam].cont.push_back(current_box);
				bins[box_target_seam].col = box_target_col;
			} else if (box_target_col != -1U) {
				//"create new column" =>
				for (unsigned int b = 0; b < bins.size(); ++b) {
					if (bins[b].col >= box_target_col) {
						bins[b].col += 1;
					}
				}
				bins.push_back(LayerBin());
				bins.back().col = box_target_col;
				bins.back().cont.push_back(current_box);
			}
			dragging_box = false;
			current_box = NULL;
			layout_boxes();
			make_closest_wanted();
			return true;
		}
	}
	return false;
}

void StackingConstraint::update(float elapsed) {

	if (boxes.size() != layer_colors().size()) {
		bins.clear();
		boxes.clear();
		boxes.resize(layer_colors().size());
		bins.resize(layer_colors().size());
		for (unsigned int l = 0; l < layer_colors().size(); ++l) {
			boxes[l].layer = l;
			boxes[l].color = layer_colors()[l];
			bins[l].cont.push_back(&boxes[l]);
		}
		layout_boxes();
		for (unsigned int b = 0; b < boxes.size(); ++b) {
			boxes[b].pos = boxes[b].target;
		}
		current_box = NULL;
		make_closest_wanted();
	}

	//update layer box positions:
	for (unsigned int b = 0; b < boxes.size(); ++b) {
		Vector2f move = boxes[b].target - boxes[b].pos;
		{ //exponential:
			float amt = 1.0f - powf(0.5f, elapsed * 2.0f);
			boxes[b].pos += move * amt;
			move *= (1.0f - amt);
		}
		//linear:
		if (length(move) > elapsed) move = normalize(move) * elapsed;
		boxes[b].pos += move;
	}
	if (!dragging_box) { //update box under cursor:
		current_box = NULL;
		for (unsigned int b = 0; b < boxes.size(); ++b) {
			Box2f bounds;
			bounds.min = boxes[b].pos - 0.5f * LayerBoxSize;
			bounds.max = boxes[b].pos + 0.5f * LayerBoxSize;
			if (bounds.contains(cur_mouse)) {
				current_box = &(boxes[b]);
			}
		}
	}
}

Vector2f StackingConstraint::size() {
	unsigned int count = 1;
	if (layer_colors().size()) {
		count = layer_colors().size();
	}
	return make_vector(
		count * 1.0f * LayerBoxSize.x + (count - 1) * 0.2f * LayerBoxSize.x + 0.4f * LayerBoxSize.y,
		count * LayerBoxSize.y + (count - 1) * 0.1f * LayerBoxSize.y + 0.4f * LayerBoxSize.y
		);
}

void StackingConstraint::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
	glVertex2f(  0.5f * size().x, -0.5f * size().y);
	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
	glVertex2f( -0.5f * size().x, -0.5f * size().y);
	glVertex2f( -0.5f * size().x,  0.5f * size().y);
	glVertex2f(  0.5f * size().x,  0.5f * size().y);
	glEnd();


	for (vector< LayerBox >::iterator b = boxes.begin(); b != boxes.end(); ++b) {
		if (dragging_box && (&(*b) == current_box)) {
			unsigned int max_boxes = 1;
			unsigned int boxes_in_col = 0;
			for (unsigned int bn = 0; bn < bins.size(); ++bn) {
				if (bins[bn].cont.size() > max_boxes) {
					max_boxes = bins[bn].cont.size();
				}
				if (bins[bn].col == box_target_col && bn < box_target_seam) {
					boxes_in_col += 1;
				}
			}
			float col_width = LayerBoxSize.x * (1.0f + 0.2f * max_boxes);
			float right_edge = size().x * 0.5f - 0.2f * LayerBoxSize.y - col_width * box_target_col;

			if (box_target_seam <= bins.size()) {
				glBegin(GL_LINES);
				glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
				float h = size().y * -0.5f + (0.2f + 1.1f * boxes_in_col - 0.05f) * LayerBoxSize.y;
				glVertex2f(right_edge + 0.2f * col_width, h);
				glVertex2f(right_edge - 1.2f * col_width, h);
				glEnd();
			} else if (box_target_col != -1U) {
				glBegin(GL_LINES);
				glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
				glVertex2f(right_edge,-0.5f * size().y + 0.33f);
				glVertex2f(right_edge,-0.5f * size().y + 0.03f);
				glEnd();
			} else {
				glBegin(GL_LINE_LOOP);
				glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
				glVertex2f(b->target.x+0.5f*LayerBoxSize.x,b->target.y+0.5f*LayerBoxSize.y);
				glVertex2f(b->target.x+0.5f*LayerBoxSize.x,b->target.y-0.5f*LayerBoxSize.y);
				glVertex2f(b->target.x-0.5f*LayerBoxSize.x,b->target.y-0.5f*LayerBoxSize.y);
				glVertex2f(b->target.x-0.5f*LayerBoxSize.x,b->target.y+0.5f*LayerBoxSize.y);
				glEnd();
			}
			continue;
		}
		glBegin(GL_QUADS);
		glColor(b->color);
		glVertex2f(b->pos.x+0.5f*LayerBoxSize.x,b->pos.y+0.5f*LayerBoxSize.y);
		glColor(lerp(b->color, make_vector(1.0f, 1.0f, 1.0f), 0.3f));
		glVertex2f(b->pos.x+0.5f*LayerBoxSize.x,b->pos.y-0.5f*LayerBoxSize.y);
		glColor(lerp(b->color, make_vector(0.3f, 0.3f, 0.3f), 0.3f));
		glVertex2f(b->pos.x-0.5f*LayerBoxSize.x,b->pos.y-0.5f*LayerBoxSize.y);
		glVertex2f(b->pos.x-0.5f*LayerBoxSize.x,b->pos.y+0.5f*LayerBoxSize.y);
		glEnd();
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


	Graphics::gl_errors("draw");
}

void StackingConstraint::layout_boxes() {
	{ //remap columns to compress.
		vector< unsigned int > new_cols;
		for (unsigned int b = 0; b < bins.size(); ++b) {
			while (bins[b].col >= new_cols.size()) {
				new_cols.push_back(-1U);
			}
			new_cols[bins[b].col] = 0;
		}
		unsigned int next_col = 0;
		for (unsigned int c = 0; c < new_cols.size(); ++c) {
			if (new_cols[c] != -1U) {
				new_cols[c] = next_col;
				++next_col;
			}
		}
		for (unsigned int b = 0; b < bins.size(); ++b) {
			assert(bins[b].col < new_cols.size());
			assert(new_cols[bins[b].col] != -1U);
			bins[b].col = new_cols[bins[b].col];
		}
	}
	unsigned int max_boxes = 1;
	for (unsigned int b = 0; b < bins.size(); ++b) {
		if (bins[b].cont.size() > max_boxes) {
			max_boxes = bins[b].cont.size();
		}
	}
	float col_width = LayerBoxSize.x * (1.0f + 0.2f * max_boxes);
	vector< float > bottom_edges;
	for (unsigned int b = 0; b < bins.size(); ++b) {
		while (bins[b].col >= bottom_edges.size()) {
			bottom_edges.push_back(-0.5f * size().y + 0.2f * LayerBoxSize.y);
		}
		float right_edge = size().x * 0.5f - 0.2f * LayerBoxSize.y - col_width * bins[b].col;
		assert(!bins[b].cont.empty());
		for (unsigned int c = 0; c < bins[b].cont.size(); ++c) {
			bins[b].cont[c]->target.x = right_edge - 0.5f * LayerBoxSize.x;
			right_edge -= 0.2f * LayerBoxSize.x;
			bins[b].cont[c]->target.y = bottom_edges[bins[b].col] + 0.5f * LayerBoxSize.y;
			if (c & 1) {
				bins[b].cont[c]->target.y += 0.05f * LayerBoxSize.y;
			}
		}
		bottom_edges[bins[b].col] += 1.1f * LayerBoxSize.y;
	}
}

void enforce_order(vector< uint32_t > form_bins, vector< uint32_t > &order) {
	vector< vector< uint32_t > > bins;
	uint32_t extracted = 0;
	for (unsigned int i = 0; i < order.size(); ++i) {
		assert(order[i] < form_bins.size());
		if (form_bins[order[i]] == -1U) continue;
		while (bins.size() <= form_bins[order[i]]) {
			bins.push_back(vector< uint32_t >());
		}
		bins[form_bins[order[i]]].push_back(order[i]);
		extracted |= (1 << i);
	}
	for (unsigned int i = order.size()-1; i < order.size(); --i) {
		if ((extracted & (1 << i)) == 0) continue;
		while (!bins.empty() && bins.back().empty()) {
			bins.pop_back();
		}
		assert(!bins.empty());
		order[i] = bins.back().back();
		bins.back().pop_back();
	}
	assert(bins.size() == 1 && bins.back().empty());
}

void StackingConstraint::make_closest_wanted() {
	vector< vector< uint32_t > > forms_bins;
	{ //extract forms from the layers dialog:
		vector< uint32_t > bins_in_col;
		for (vector< LayerBin >::iterator bin = bins.begin(); bin != bins.end(); ++bin) {
			while (forms_bins.size() <= bin->col) {
				forms_bins.push_back(vector< uint32_t >(layer_colors().size(), -1U));
			}
			while (bins_in_col.size() <= bin->col) {
				bins_in_col.push_back(0);
			}
			for (vector< LayerBox * >::iterator box = bin->cont.begin(); box != bin->cont.end(); ++box) {
				assert((*box)->layer < forms_bins[bin->col].size());
				forms_bins[bin->col][(*box)->layer] = bins_in_col[bin->col];
			}
			bins_in_col[bin->col] += 1;
		}
	}
	size_t stackings = count_stackings(layer_colors().size());
	closest_wanted().clear();
	closest_wanted().resize(stackings, -1U);
	for (unsigned int s = 0; s < stackings; ++s) {
		vector< uint32_t > order = to_stacking(s, layer_colors().size());
		for (unsigned int f = forms_bins.size()-1; f < forms_bins.size(); --f) {
			enforce_order(forms_bins[f], order);
		}
		closest_wanted()[s] = to_stacking_index(order);
	}
	//--------------------------
	//And dump the table for good measure:
	std::cout << "Mapping due to constraints:\n";
	for (unsigned int s = 0; s < stackings; ++s) {
		{
			vector< uint32_t > order = to_stacking(s, layer_colors().size());
			for (unsigned int i = 0; i < order.size(); ++i) {
				if (i) std::cout << '/';
				std::cout << order[i];
			}
		}
		std::cout << " -> ";
		{
			vector< uint32_t > order = to_stacking(closest_wanted()[s], layer_colors().size());
			for (unsigned int i = 0; i < order.size(); ++i) {
				if (i) std::cout << '/';
				std::cout << order[i];
			}
		}
		std::cout << "\n";
	}
	std::cout.flush();
}
