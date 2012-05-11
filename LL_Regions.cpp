#include "LL_Regions.hpp"

#include "LL_LayeringModule.hpp"

#include <Graphics/GLSLShader.hpp>
#include <Vector/VectorGL.hpp>

#include <cassert>
#include <algorithm>

using std::sort;
using std::swap;

REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )

namespace {
	Module *create_module(const std::string &params_in) {
		return new LayersRegions();
	}
	class Fred {
	public:
		Fred() {
			register_module("ll_regions", create_module, "");
		}
	} fred;
}

void LayerBox::update_pos( float elapsed_time ) {
	if (pos > target_pos) {
		pos -= elapsed_time * BoxHeight * 2.0f;
		if (pos < target_pos) pos = target_pos;
	} else {
		pos += elapsed_time * BoxHeight * 2.0f;
		if (pos > target_pos) pos = target_pos;
	}
	if (target_disp) {
		if (pos < target_pos) {
			scale += elapsed_time;
			if (scale >= 1.0f) scale = 1.0f;
		} else {
			float amt = 1.0f - (pos - target_pos) / BoxHeight;
			if (amt < 0.5f) amt = 0.5f;
			if (scale < amt) {
				scale += elapsed_time;
				if (scale > amt) scale = amt;
			} else {
				scale -= elapsed_time;
				if (scale < amt) scale = amt;
			}
		}
		fade += elapsed_time;
		if (fade >= 1.0f) fade = 1.0f;
	} else {
		scale -= elapsed_time;
		if (scale < 0.0f) scale = 0.0f;
		fade -= elapsed_time;
		if (fade < 0.0f) fade = 0.0f;
	}
}

GraphArrow::GraphArrow() : ind((unsigned)-1), loc(make_vector(0.0f, 0.0f)) {
}

void GraphArrow::update(float elapsed_time) {
	for (unsigned int b = 0; b < boxes.size(); ++b) {
		boxes[b].update_pos(elapsed_time);
	}
}

bool GraphArrow::update_location(LayeringModule &layering, Vector2f _loc) {
	loc = _loc;
	unsigned int c = (int)((loc.x / layering.size().x + 0.5f) * layering.width + 0.5f);
	unsigned int r = (int)((loc.y / layering.size().y + 0.5f) * layering.height + 0.5f);
	if (r >= layering.width || c >= layering.width) return false;
	ind = r * layering.width + c;
	if (ind >= layering.tags.size()) return false;
	unsigned int tag = layering.tags[ind];
	if (tag >= layering.stacking.lists.size()) return false;
	vector< unsigned int > const &list = layering.stacking.lists[tag];
	for (unsigned int b = 0; b < boxes.size(); ++b) {
		boxes[b].target_pos = boxes[b].pos;
		boxes[b].target_disp = false;
	}

	for (unsigned int i = 0; i < list.size(); ++i) {
		unsigned int layer = list[i];

		float pos = ArrowHeight + ShaftInset + (ShaftInset + BoxHeight) * i + BoxHeight * 0.5f;
		uint32_t col = layering.layers[layer][ind];
		Vector4f color = make_vector((col & 0xff) / 255.0f, ((col >> 8) & 0xff) / 255.0f, ((col >> 16) & 0xff) / 255.0f, ((col >> 24) & 0xff) / 255.0f);
		if (layer < layering.colors.size()) {
			color = make_vector(layering.colors[layer], 1.0f);
		}

		bool found = false;
		for (unsigned int b = 0; b < boxes.size(); ++b) {
			if (boxes[b].layer == layer) {
				boxes[b].color = color;
				boxes[b].target_pos = pos;
				boxes[b].target_disp = true;
				found = true;
				break;
			}
		}
		if (!found) {
			boxes.push_back(LayerBox());
			boxes.back().color = color;
			boxes.back().layer = layer;
			boxes.back().pos = pos;
			boxes.back().scale = 0.0f;
			boxes.back().fade = 0.0f;
			boxes.back().target_pos = pos;
			boxes.back().target_disp = true;
		}
	}
	sort(boxes.begin(), boxes.end());

	return true;
}





void draw_nothing(const PortData *, const Connection &, void *) {
}


LayersRegions::LayersRegions() {
	in_ports.push_back(&layering);
	layering.name = "layers";
	layering.position = make_vector(0.0f, 0.0f);
	layering.draw_fn = draw_nothing;
	layering() = NULL;

	out_ports.push_back(&top_state);
	top_state.name = "state";
	top_state.position = make_vector(0.0f, 0.5f);
	top_state.draw_fn = draw_nothing;
	top_state() = NULL;

	mouse_pos = make_vector(10.0f, 0.0f);
	dragging = NULL;

	draw_graph = false;
	draw_graph_tween = 0.0f;

	last_region = (unsigned)-1;
	last_layer = (unsigned)-1;
}

LayersRegions::~LayersRegions() {
}

Vector2f LayersRegions::size() {
	if (!layering() || layering()->width == 0 || layering()->height == 0) {
		return make_vector(0.3f, 1.0f);
	} else {
		return make_vector(1.0f * layering()->width / float(layering()->height), 1.0f);
	}
}

namespace {
Graphics::ProgramObjectRef regions_shader = NULL;
}

void LayersRegions::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	if (!regions_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_regions.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		regions_shader = Graphics::get_program_object(frag);
		assert(regions_shader.ref);
		glUseProgramObjectARB(regions_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(regions_shader->handle, "tags"), 0);
		glUseProgramObjectARB(0);
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor3f(0.4f, 0.2f, 0.2f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	if (!layering()) {
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		return;
	}

	static unsigned int width = layering()->width;
	static unsigned int height = layering()->height;
	
	glUseProgramObjectARB(regions_shader->handle);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, layering()->get_tags_tex());
	glUniform4fARB(glGetUniformLocationARB(regions_shader->handle, "highlight"), 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform3fARB(glGetUniformLocationARB(regions_shader->handle, "highlight_color"), 1.0f, 0.2f, 0.7f);
	float px_size = 2.0f / Graphics::screen_y * viewport.size().x / screen_viewport.size().x * height / size().y;
	glUniform1fARB(glGetUniformLocationARB(regions_shader->handle, "size"), px_size * 0.5);
	glUniform1fARB(glGetUniformLocationARB(regions_shader->handle, "px_size"), px_size);


	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0,0);
	glVertex2f(-0.5f*size().x,-0.5f*size().y);
	glTexCoord2f(width,0);
	glVertex2f( 0.5f*size().x,-0.5f*size().y);
	glTexCoord2f(width,height);
	glVertex2f( 0.5f*size().x, 0.5f*size().y);
	glTexCoord2f(0,height);
	glVertex2f(-0.5f*size().x, 0.5f*size().y);
	glEnd();
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glUseProgramObjectARB(0);

	const float DepScl = 0.5f;

	vector< GraphArrow * > list_arrows(layering()->stacking.lists.size());
	for (unsigned int a = 0; a < arrows.size(); ++a) {
		if (arrows[a].ind >= layering()->tags.size()) continue;
		uint32_t tag = layering()->tags[arrows[a].ind];
		if (tag > list_arrows.size()) continue;
		list_arrows[tag] = &arrows[a];
	}
	if (draw_graph_tween) { //list graph... graph-y bits.
		vector< vector< unsigned int > > const &edges = layering()->stacking.edges;
		assert(edges.size() == list_arrows.size());
		for (unsigned int a = 0; a < list_arrows.size(); ++a) {
			if (list_arrows[a] == NULL) continue;
			for (unsigned int e = 0; e < edges[a].size(); ++e) {
				if (edges[a][e] >= list_arrows.size()) continue;
				if (list_arrows[edges[a][e]] == NULL) continue;
				Vector2f loc1 = list_arrows[a]->loc;
				Vector2f loc2 = list_arrows[edges[a][e]]->loc;
				loc1.y /= DepScl;
				loc2.y /= DepScl;
				Vector2f to = normalize(loc2 - loc1);
				Vector2f perp = perpendicular(to);
				perp *= 0.004;
				to *= 0.004;
				loc1.y *= DepScl;
				loc2.y *= DepScl;
				perp.y *= DepScl;
				to.y *= DepScl;
				loc2 = loc1 + (loc2 - loc1) * draw_graph_tween;
				glBegin(GL_QUADS);
				glColor4f(0.0f, 0.0f, 0.01f, 1.0f);
				glVertex(loc1 - perp);
				glVertex(loc1 + perp);
				glColor4f(0.3f, 0.0f, 0.01f, 0.0f);
				glVertex(loc2 + perp);
				glVertex(loc2 - perp);
				glEnd();
				glBegin(GL_LINES);
				glColor4f(0.0f, 0.0f, 0.01f, 1.0f);
				glVertex(loc1 - perp);
				glColor4f(0.3f, 0.0f, 0.01f, 0.0f);
				glVertex(loc2 - perp);
				glColor4f(0.0f, 0.0f, 0.01f, 1.0f);
				glVertex(loc1 + perp);
				glColor4f(0.3f, 0.0f, 0.01f, 0.0f);
				glVertex(loc2 + perp);
				glEnd();
			}
		}
		//and again, for recursion arrows:
		for (unsigned int a = 0; a < list_arrows.size(); ++a) {
			if (list_arrows[a] == NULL) continue;
			for (unsigned int e = 0; e < edges[a].size(); ++e) {
				if (edges[a][e] >= list_arrows.size()) continue;
				if (list_arrows[edges[a][e]] == NULL) continue;
				Vector2f loc1 = list_arrows[a]->loc;
				Vector2f loc2 = list_arrows[edges[a][e]]->loc;
				loc1.y /= DepScl;
				loc2.y /= DepScl;
				Vector2f to = normalize(loc2 - loc1);
				Vector2f perp = perpendicular(to);
				perp *= 0.004;
				to *= 0.004;
				loc1.y *= DepScl;
				loc2.y *= DepScl;
				perp.y *= DepScl;
				to.y *= DepScl;
				if (stack.size() && stack.back()->region == a && stack.back()->state == RECURSE) {
					Vector2f m = loc1 + (loc2 - loc1) * 0.5f;
					glBegin(GL_QUAD_STRIP);
					glColor3f(1.0f, 0.2f, 0.0f);
					glVertex(loc1 - perp * 2.0f);
					glVertex(loc1 + perp * 2.0f);
					glVertex(m - perp * 2.0f);
					glVertex(m + perp * 2.0f);
					glVertex(m - perp * 5.0f);
					glVertex(m + perp * 5.0f);
					glVertex(m + to * 10.0f);
					glVertex(m + to * 10.0f);
					glEnd();
					glBegin(GL_LINE_STRIP);
					glColor3f(1.0f, 0.2f, 0.0f);
					glVertex(loc1 - perp * 2.0f);
					glVertex(m - perp * 2.0f);
					glVertex(m - perp * 5.0f);
					glVertex(m + to * 10.0f);
					glVertex(m + perp * 5.0f);
					glVertex(m + perp * 2.0f);
					glVertex(loc1 + perp * 2.0f);
					glEnd();
				}
			}
		}
	}

	{//Figure out where FlipStates are:
		Vector2f base = size() * 0.5f;
		base.x -= ShaftInset;
		base.y -= ShaftInset;
		//now build up stacks per-arrow:
		for (unsigned int s = 0; s < stack.size(); ++s) {
			stack[s]->pos.max = base;
			Vector2f size = make_vector(StateWidth, StateHeight) * ((s + 1 == stack.size())?ActiveScale:1.0f);
			stack[s]->pos.min = stack[s]->pos.max - size;
			base.y -= size.y + ShaftInset;
		}
	}

	for (unsigned int a = 0; a < arrows.size(); ++a) {
		GraphArrow const &arrow = arrows[a];
		Vector2f at = arrow.loc;

		unsigned int tag = (unsigned)-1;
		if (arrow.ind < layering()->tags.size()) {
			tag = layering()->tags[arrow.ind];
		}
		//Draw a line to any stack stuff that cares about us:
		for (unsigned int s = 0; s < stack.size(); ++s) {
			FlipState &state = *stack[s];
			if (state.region != tag) continue;

			//decently curve-ish path:
			Vector2f to;
			to.x = state.pos.min.x;
			to.y = state.pos.center().y;
			Vector2f from;
			from.x = at.x;
			from.y = at.y;
			if (s + 1 != stack.size()) {
				glLineWidth(1.0f);
			} else {
				glLineWidth(3.0f);
			}
			glBegin(GL_LINE_STRIP);
			glColor4f(0.0f, 0.0f, 0.01f, 0.2f);
			glVertex2f(from.x, from.y);
			glColor4f(0.0f, 0.0f, 0.01f, 0.0f);
			glVertex2f(from.x + StateWidth, from.y);
			glEnd();
			if (s + 1 != stack.size()) {
				glColor4f(0.7f, 0.9f, 0.1f, 0.5f);
			} else {
				glColor4f(1.0f, 0.3f, 0.1f, 0.9f);
			}
			glBegin(GL_LINE_STRIP);
			for (unsigned int i = 0; i <= 20; ++i) {
				float amt = i / 20.0f;
				Vector2f a = from;
				a.x += StateWidth;
				Vector2f b = to;
				b.x -= StateWidth;
				Vector2f fa = from + (a - from) * amt;
				Vector2f ab = a + (b - a) * amt;
				Vector2f bt = b + (to - b) * amt;
				Vector2f fab = fa + (ab - fa) * amt;
				Vector2f abt = ab + (bt - ab) * amt;
				glVertex(fab + (abt - fab) * amt);
			}
			glEnd();
			glLineWidth(1.0f);
		}


		float shaft_height = ShaftInset;
		for (unsigned int b = 0; b < arrow.boxes.size(); ++b) {
			if (!arrow.boxes[b].target_disp) continue;
			shaft_height += BoxHeight + ShaftInset;
		}
		Vector2f l4 = make_vector( 0.5f * (ShaftWidth - ShaftInset), ArrowHeight + shaft_height);
		Vector2f l3 = make_vector( 0.5f * ShaftWidth, ArrowHeight + shaft_height - 0.5f * ShaftInset);
		Vector2f l2 = make_vector( 0.5f * ShaftWidth, ArrowHeight + 0.5f * ShaftInset);
		Vector2f l1 = make_vector( 0.5f * (ShaftWidth - ShaftInset), ArrowHeight);
		Vector2f l0 = make_vector( 0.5f * ArrowWidth, ArrowHeight);
		
		glBegin(GL_QUAD_STRIP);
		glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
		glVertex2f(at.x-l0.x+l0.y*0.3f,at.y+l0.y*0.5f); glVertex2f(at.x+l0.x+l0.y*0.3f,at.y+l0.y*0.5f);
		glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
		glVertex2f(at.x,at.y); glVertex2f(at.x,at.y);
		glEnd();

		glBegin(GL_QUAD_STRIP);
		glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
		glVertex2f(at.x-l4.x,at.y+l4.y); glVertex2f(at.x+l4.x,at.y+l4.y);
		glVertex2f(at.x-l3.x,at.y+l3.y); glVertex2f(at.x+l3.x,at.y+l3.y);
		glVertex2f(at.x-l2.x,at.y+l2.y); glVertex2f(at.x+l2.x,at.y+l2.y);
		glVertex2f(at.x-l1.x,at.y+l1.y); glVertex2f(at.x+l1.x,at.y+l1.y);
		glVertex2f(at.x-l0.x,at.y+l0.y); glVertex2f(at.x+l0.x,at.y+l0.y);
		glVertex2f(at.x,at.y); glVertex2f(at.x,at.y);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor3f(0.7f, 0.7f, 0.7f);
		glVertex2f(at.x-l4.x,at.y+l4.y);
		glVertex2f(at.x-l3.x,at.y+l3.y);
		glVertex2f(at.x-l2.x,at.y+l2.y);
		glVertex2f(at.x-l1.x,at.y+l1.y);
		glVertex2f(at.x-l0.x,at.y+l0.y);
		glVertex2f(at.x,at.y);
		glVertex2f(at.x+l0.x,at.y+l0.y);
		glVertex2f(at.x+l1.x,at.y+l1.y);
		glVertex2f(at.x+l2.x,at.y+l2.y);
		glVertex2f(at.x+l3.x,at.y+l3.y);
		glVertex2f(at.x+l4.x,at.y+l4.y);
		glEnd();


		for (unsigned int b = 0; b < arrow.boxes.size(); ++b) {
			LayerBox const &box = arrow.boxes[b];
			if (box.fade == 0.0f) continue;
			glColor4f(box.color.x, box.color.y, box.color.z, box.color.w * box.fade);
			Box2f ext = box.get_box();
			ext.min += arrow.loc;
			ext.max += arrow.loc;
			glBegin(GL_QUADS);
			glVertex2f(ext.min.x, ext.min.y);
			glVertex2f(ext.max.x, ext.min.y);
			glVertex2f(ext.max.x, ext.max.y);
			glVertex2f(ext.min.x, ext.max.y);
			glEnd();
			if (stack.empty() || stack.back()->region != tag) continue;
			FlipState &state = *stack.back();
			bool show = false;
			if (state.state & ShowX) {
				if (box.layer == state.x) show = true;
			}
			if (state.state & ShowT) {
				if (box.layer == state.t) show = true;
			}
			if (state.state & ShowY) {
				if (b - 1 < arrow.boxes.size()) {
					if (arrow.boxes[b-1].target_disp && arrow.boxes[b-1].layer == state.x) {
						show = true;
					}
				}
			}
			if (state.state & ShowM) {
				if (b + 1 < arrow.boxes.size()) {
					if (arrow.boxes[b+1].target_disp && arrow.boxes[b+1].layer == state.x) {
						show = true;
					}
				}
			}
			if (show) {
				glLineWidth(2.0f);
				glColor3f(1.0f, 0.2f, 0.1f);
				glBegin(GL_LINE_LOOP);
				glVertex2f(ext.min.x, ext.min.y);
				glVertex2f(ext.min.x, ext.max.y);
				glVertex2f(ext.max.x, ext.max.y);
				glVertex2f(ext.max.x, ext.min.y);
				glEnd();
				glLineWidth(1.0f);
			}
			if (state.state == SWAP_ACTION && box.layer == state.x && b + 1 < arrow.boxes.size()) {
				Box2f ext2 = arrow.boxes[b+1].get_box();
				ext2.min += arrow.loc;
				ext2.max += arrow.loc;
				Vector2f a;
				a.x = ext.min.x;
				a.y = ext.center().y;
				Vector2f b;
				b.x = ext2.min.x;
				b.y = ext2.center().y;
				glLineWidth(3.0f);
				glBegin(GL_LINE_STRIP);
				glColor3f(1.0f, 0.2f, 0.1f);
				for (unsigned int i = 0; i <= 17; ++i) {
					float ang = (i / 17.0f) * float(M_PI) + float(M_PI) * 0.5f;
					glVertex(0.5f * (a + b) + (b.y-a.y) * 0.5f * make_vector(cosf(ang), sinf(ang)));
				}
				glEnd();
				glLineWidth(1.0f);
			}
		}

	}

	//draw all FlipStates:
	for (unsigned int s = 0; s < stack.size(); ++s) {
		FlipState const &state = *stack[s];
		glBegin(GL_QUAD_STRIP);
		glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
		glVertex2f(state.pos.min.x, state.pos.min.y);
		glVertex2f(state.pos.max.x, state.pos.min.y);
		glVertex2f(state.pos.min.x, state.pos.max.y);
		glVertex2f(state.pos.max.x, state.pos.max.y);
		glEnd();
		{ //squares:
			float s = state.pos.max.y - state.pos.min.y - ShaftInset;
			glBegin(GL_QUADS);
			glColor(state.color_x);
			glVertex(state.pos.min + make_vector(0.5f * ShaftInset, 0.5f * ShaftInset));
			glVertex(state.pos.min + make_vector(0.5f * ShaftInset+s, 0.5f * ShaftInset));
			glVertex(state.pos.min + make_vector(0.5f * ShaftInset+s, 0.5f * ShaftInset+s));
			glVertex(state.pos.min + make_vector(0.5f * ShaftInset, 0.5f * ShaftInset+s));
			glColor(state.color_t);
			glVertex(state.pos.max - make_vector(0.5f * ShaftInset, 0.5f * ShaftInset));
			glVertex(state.pos.max - make_vector(0.5f * ShaftInset+s, 0.5f * ShaftInset));
			glVertex(state.pos.max - make_vector(0.5f * ShaftInset+s, 0.5f * ShaftInset+s));
			glVertex(state.pos.max - make_vector(0.5f * ShaftInset, 0.5f * ShaftInset+s));
			glEnd();
		}
		if (s + 1 == stack.size()) {
			glColor3f(1.0f, 0.2f, 0.1f);
			glLineWidth(3.0f);
		} else {
			glColor3f(0.3f, 0.3f, 0.3f);
			glLineWidth(1.0f);
		}
		glBegin(GL_LINE_LOOP);
		glVertex2f(state.pos.min.x, state.pos.min.y);
		glVertex2f(state.pos.min.x, state.pos.max.y);
		glVertex2f(state.pos.max.x, state.pos.max.y);
		glVertex2f(state.pos.max.x, state.pos.min.y);
		glEnd();
		glLineWidth(1.0f);
		
	}


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}
void LayersRegions::update(float elapsed_time) {
	if (layering() == NULL) return;
	for (unsigned int a = 0; a < arrows.size(); ++a) {
		arrows[a].update(elapsed_time);
	}
	if (draw_graph) {
		draw_graph_tween += elapsed_time;
		if (draw_graph_tween >= 1.0f) {
			draw_graph_tween = 1.0f;
		}
	} else {
		draw_graph_tween -= elapsed_time;
		if (draw_graph_tween <= 0.0f) {
			draw_graph_tween = 0.0f;
		}
	}
}

bool LayersRegions::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (layering() == NULL) return false;
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button != SDL_BUTTON_LEFT && event.button.button != SDL_BUTTON_RIGHT) {
		return false;
	}
	if (dragging) {
		if (event.type == SDL_MOUSEMOTION) {
			if (!dragging->update_location(*layering(), dragging->loc + local_mouse - mouse_pos)) {
				for (unsigned int a = 0; a < arrows.size(); ++a) {
					if (&arrows[a] == dragging) {
						arrows.erase(arrows.begin()+a);
						dragging = NULL;
						break;
					}
				}
			} else {
				Vector2f loc = dragging->loc;
				dragging = NULL;
				sort(arrows.begin(), arrows.end());
				for (unsigned int a = 0; a < arrows.size(); ++a) {
					if (arrows[a].loc == loc) {
						dragging = &arrows[a];
						break;
					}
				}
			}
			mouse_pos = local_mouse;
			return true;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			return true;
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			dragging = NULL;
			return true;
		}
		return false;
	}
	mouse_pos = local_mouse;

	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_g) {
		draw_graph = !draw_graph;
		return true;
	}

	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		step_execution();
		return true;
	}

	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DELETE) {
		clear_execution();
		return true;
	}

	if (event.type == SDL_MOUSEBUTTONDOWN) {
		//(a) see if I'm grabbing an arrow.
		for (unsigned int a = arrows.size()-1; a < arrows.size(); --a) {
			GraphArrow const &arrow = arrows[a];
			{ //check tip:
				float h = local_mouse.y - arrow.loc.y;
				if (h >= 0 && h <= ArrowHeight) {
					float w = h / ArrowHeight * ArrowWidth;
					if (fabsf(local_mouse.x - arrow.loc.x) < 0.5f * w) {
						dragging = &arrows[a];
						return true;
					}
				}
			}
			if (arrow.ind >= layering()->tags.size()) continue;
			uint32_t tag = layering()->tags[arrow.ind];
			if (tag >= layering()->stacking.lists.size()) continue;
			vector< unsigned int > &list = layering()->stacking.lists[tag];
			for (unsigned int i = 0; i < list.size(); ++i) {
				for (unsigned int b = 0; b < arrow.boxes.size(); ++b) {
					if (arrow.boxes[b].layer != list[i]) continue;
					Box2f box = arrow.boxes[b].get_box();
					box.min += arrow.loc;
					box.max += arrow.loc;
					if (!box.contains(local_mouse)) break;
					if (event.button.button == SDL_BUTTON_RIGHT) {
						if (last_region != tag) {
							last_region = tag;
							last_layer = list[i];
						} else {
							start_execution(tag, last_layer, list[i]);
							last_region = (unsigned)-1;
							last_layer = (unsigned)-1;
						}
						return true;
					}
					if (local_mouse.y > box.center().y) {
						if (i + 1 < list.size()) {
							swap(list[i], list[i+1]);
							layering()->update_image();
							if (!arrows[a].update_location(*layering(), arrow.loc)) {
								std::cout << "WATCH OUT! Errow Arror!" << std::endl;
							}
						}
					} else {
						if (i - 1 < list.size()) {
							swap(list[i], list[i-1]);
							layering()->update_image();
							if (!arrows[a].update_location(*layering(), arrow.loc)) {
								std::cout << "WATCH OUT! Errow Arror!" << std::endl;
							}
						}
					}
					return true;
				}
			}
		}
		if (SDL_GetModState() & KMOD_SHIFT) { //(b) add an arrow?
			arrows.push_back(GraphArrow());
			if (!arrows.back().update_location(*layering(), local_mouse)) {
				arrows.pop_back();
			} else {
				sort(arrows.begin(), arrows.end());
			}
		}
	}
	return false;
}

void LayersRegions::start_execution(unsigned int region, unsigned int x, unsigned int t) {
	clear_execution();
	std::cout << "Starting execution" << std::endl;
	assert(layering());
	assert(region < layering()->stacking.lists.size());
	stack.push_back(new FlipState(region, x, t));
	for (unsigned int a = 0; a < arrows.size(); ++a) {
		GraphArrow const &arrow = arrows[a];
		unsigned int tag = (unsigned)-1;
		if (arrow.ind < layering()->tags.size()) {
			tag = layering()->tags[arrow.ind];
		}
		if (tag != region) continue;
		for (unsigned int b = 0; b < arrow.boxes.size(); ++b) {
			if (arrow.boxes[b].layer == x) {
				stack.back()->color_x = arrow.boxes[b].color;
			}
			if (arrow.boxes[b].layer == t) {
				stack.back()->color_t = arrow.boxes[b].color;
			}
		}
	}
	top_state() = stack.back();
}

void LayersRegions::clear_execution() {
	std::cout << "Clearing execution" << std::endl;
	for (unsigned int i = 0; i < stack.size(); ++i) {
		delete stack[i];
	}
	stack.clear();
	top_state() = NULL;
}

void LayersRegions::step_execution() {
	if (!layering()) {
		clear_execution();
		return;
	}
	if (stack.empty()) return;
	FlipState *state = stack.back();
	if (state->region >= layering()->stacking.lists.size()) {
		clear_execution();
		return;
	}
	vector< unsigned int > &list = layering()->stacking.lists[state->region];
	vector< unsigned int > const &edges = layering()->stacking.edges[state->region];
	unsigned int x_ind = (unsigned)-1;
	unsigned int t_ind = (unsigned)-1;
	for (unsigned int i = 0; i < list.size(); ++i) {
		if (list[i] == state->x) {
			x_ind = i;
		}
		if (list[i] == state->t) {
			t_ind = i;
		}
	}
	if (state->state == INIT) {
		state->state = CHECK;
	} else if (state->state == CHECK) {
		if (x_ind < list.size() && t_ind < list.size()) {
			state->state = WHILE;
		} else {
			state->state = CHECK_RETURN;
		}
	} else if (state->state == CHECK_RETURN) {
		stack.pop_back();
		delete state;
		state = NULL;
		if (stack.empty()) top_state() = NULL;
		else top_state() = stack.back();
	} else if (state->state == WHILE) {
		if (x_ind < t_ind) {
			state->state = SWAP;
		} else {
			state->state = WHILE_RETURN;
		}
	} else if (state->state == WHILE_RETURN) {
		stack.pop_back();
		delete state;
		state = NULL;
		if (stack.empty()) top_state() = NULL;
		else top_state() = stack.back();
	} else if (state->state == SWAP) {
		state->state = SWAP_ACTION;
	} else if (state->state == SWAP_ACTION) {
		if (x_ind + 1 >= list.size()) {
			std::cout << "Swap problem." << std::endl;
			clear_execution();
			return;
		}
		swap(list[x_ind], list[x_ind+1]);
		for (unsigned int a = 0; a < arrows.size(); ++a) {
			if (!arrows[a].update_location(*layering(), arrows[a].loc)) {
				std::cout << "Update problem." << std::endl;
			}
		}
		if (layering()) {
			layering()->update_image();
		}
		state->state = SWAP_AFTER;
	} else if (state->state == SWAP_AFTER) {
		state->state = RECURSE;
	} else if (state->state == RECURSE) {
		if (x_ind-1 >= list.size()) {
			std::cout << "Swap problem before recurse?" << std::endl;
			clear_execution();
		}
		for (unsigned int e = 0; e < edges.size(); ++e) {
			if (layering()->stacking.lists[edges[e]].size() < 2) continue; //ignore short lists.
			stack.push_back(new FlipState(edges[e], state->x, list[x_ind-1]));
			stack.back()->color_x = state->color_x;
			bool have_dest = false;
			for (unsigned int a = 0; a < arrows.size(); ++a) {
				GraphArrow const &arrow = arrows[a];
				unsigned int tag = (unsigned)-1;
				if (arrow.ind < layering()->tags.size()) {
					tag = layering()->tags[arrow.ind];
				}
				if (tag != state->region && tag != edges[e]) continue;
				for (unsigned int b = 0; b < arrow.boxes.size(); ++b) {
					if (arrow.boxes[b].target_disp == false) continue;
					if (arrow.boxes[b].layer == stack.back()->t) {
						if (tag == edges[e]) {
							have_dest = true;
							stack.back()->color_t = arrow.boxes[b].color;
						} else if (!have_dest) {
							stack.back()->color_t = arrow.boxes[b].color;
						}
					}
				}
			}
			top_state() = stack.back();
		}
		state->state = RECURSE_AFTER;
	} else if (state->state == RECURSE_AFTER) {
		state->state = WHILE;
	} else {
		std::cout << "Unknown state." << std::endl;
		clear_execution();
	}
}

