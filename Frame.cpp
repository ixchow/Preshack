#include "Frame.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Vector/VectorGL.hpp>

#include <assert.h>

#include <iostream>
#include <set>

using std::cout;
using std::endl;

using std::make_pair;
using std::set;

REQUIRE_GL_EXTENSION(GL_ARB_texture_float);

Frame::Frame()
		:	top_color(make_vector(0.4f, 0.4f, 0.4f)),
			bottom_color(make_vector(0.1f, 0.1f, 0.1f)),
			grad_tex(0),
			area(make_box(make_vector(-Graphics::aspectf(),-1.0f),make_vector(Graphics::aspectf(),1.0f))),
			screen_height(2.0f),
			screen_center(make_vector(0.0f, 0.0f)),
			tween_in(0.0f),
			tween(0.0f),
			tween_target(NULL),
			box(make_box(make_vector(0.0f, 0.0f), make_vector(0.0f, 0.0f))),
			show_box(false),
			box_fade(0.0f),
			preview(make_box(make_vector(-Graphics::aspectf(),-1.0f),make_vector(Graphics::aspectf(),1.0f))) {
	previews.push_back(preview);
}

float ease_size(float amt) {
	if (amt < 0.0f) return 0.0f;
	if (amt > 1.0f) return 1.0f;
	return 0.5f - 0.5f * cosf(amt * float(M_PI));
}
float ease_pos(float amt) {
	if (amt < 0.0f) return 0.0f;
	if (amt > 1.0f) return 1.0f;
	return 0.5f - 0.5f * cosf(amt * float(M_PI));
}

Vector2f Frame::size() {
	Vector2f ret = preview.max - preview.min;
	if (tween_in != 0.0f) {
		Vector2f reshaped = ret;
		if (reshaped.y * Graphics::aspectf() > reshaped.x) {
			reshaped.x = reshaped.y * Graphics::aspectf();
		} else {
			reshaped.y = reshaped.x / Graphics::aspectf();
		}
		ret += ease_size(tween_in) * (reshaped - ret);
	}
	return ret;
}

extern bool edit_mode; //Hack!

void Frame::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	Box2f internal_box, external_box;
	get_boxes(internal_box, external_box);

	{ //transform the viewport and scale info:
		float scale_x = (external_box.max.x-external_box.min.x) / (internal_box.max.x-internal_box.min.x);
		float scale_y = (external_box.max.y-external_box.min.y) / (internal_box.max.y-internal_box.min.y);
		if (scale_x > scale_y) scale *= scale_x;
		else scale *= scale_y;

		viewport.min -= external_box.min;
		viewport.max -= external_box.min;
		viewport.min.x /= scale_x;
		viewport.min.y /= scale_y;
		viewport.max.x /= scale_x;
		viewport.max.y /= scale_y;
		viewport.min += internal_box.min;
		viewport.max += internal_box.min;
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	if (grad_tex == 0) {
		glGenTextures(1, &grad_tex);
	}
	{ //DEBUG: re-upload?
		vector< Vector3f > grad(2);
		grad[0] = bottom_color;
		grad[1] = top_color;
		glBindTexture(GL_TEXTURE_1D, grad_tex);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB16F_ARB, 2, 0, GL_RGB, GL_FLOAT, &grad[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_1D, 0);
	}

	glBoxToBox(viewport, screen_viewport);
	Box2f draw_area = area;
	draw_area.min = max(draw_area.min, viewport.min);
	draw_area.max = min(draw_area.max, viewport.max);
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, grad_tex);

	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord1f(0.25f + 0.5f * (draw_area.max.y - area.min.y) / (area.max.y - area.min.y));
	glVertex2f(draw_area.min.x, draw_area.max.y);
	glVertex2f(draw_area.max.x, draw_area.max.y);
	glTexCoord1f(0.25f + 0.5f * (draw_area.min.y - area.min.y) / (area.max.y - area.min.y));
	glVertex2f(draw_area.max.x, draw_area.min.y);
	glVertex2f(draw_area.min.x, draw_area.min.y);
	glEnd();

	glBindTexture(GL_TEXTURE_1D, 0);
	glDisable(GL_TEXTURE_1D);

	/*glBegin(GL_LINES); //DEBUG: viewport 'x'.
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(viewport.min.x,viewport.min.y);
	glVertex2f(viewport.max.x,viewport.max.y);
	glVertex2f(viewport.min.x,viewport.max.y);
	glVertex2f(viewport.max.x,viewport.min.y);
	glEnd();*/

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	if (recurse > 0) {
		--recurse;
		auto last = items.end();
		auto real_i = items.begin();
		while (1) {
			auto i = real_i;
			if (real_i != items.end()) { //if still iterating items:
				++real_i; //update real index
				if (*i == tween_target) { //if currently at tween target, skin-n-save:
					last = i;
					continue;
				}
			} else { //need to do last:
				if (last != items.end()) {
					i = last;
					last = items.end();
				} else { //we've done last, so done:
					break;
				}
			}
			Box2f span;
			span.min = (*i)->position - (*i)->size() * (*i)->scale * 0.5f;
			span.max = (*i)->position + (*i)->size() * (*i)->scale * 0.5f;
			Box2f view = span;
			view.min = max(view.min, viewport.min);
			view.max = min(view.max, viewport.max);
			if (view.min.x >= view.max.x) continue;
			if (view.min.y >= view.max.y) continue;
			Box2f int_view;
			int_view.min = view.min - (*i)->position;
			int_view.max = view.max - (*i)->position;
			int_view.min /= (*i)->scale;
			int_view.max /= (*i)->scale;

			Box2f screen_view = screen_viewport;
			screen_view.min.x = (view.min.x - viewport.min.x) * (screen_viewport.max.x - screen_viewport.min.x) / (viewport.max.x - viewport.min.x) + screen_viewport.min.x;
			screen_view.max.x = (view.max.x - viewport.min.x) * (screen_viewport.max.x - screen_viewport.min.x) / (viewport.max.x - viewport.min.x) + screen_viewport.min.x;
			screen_view.min.y = (view.min.y - viewport.min.y) * (screen_viewport.max.y - screen_viewport.min.y) / (viewport.max.y - viewport.min.y) + screen_viewport.min.y;
			screen_view.max.y = (view.max.y - viewport.min.y) * (screen_viewport.max.y - screen_viewport.min.y) / (viewport.max.y - viewport.min.y) + screen_viewport.min.y;

			{ //enable clipping:
				int min_x = int((0.5f / Graphics::aspectf() * screen_viewport.min.x + 0.5f) * Graphics::screen_x + 0.5f);
				int max_x = int((0.5f / Graphics::aspectf() * screen_viewport.max.x + 0.5f) * Graphics::screen_x + 0.5f);
				int min_y = int((0.5f * screen_viewport.min.y + 0.5f) * Graphics::screen_y + 0.5f);
				int max_y = int((0.5f * screen_viewport.max.y + 0.5f) * Graphics::screen_y + 0.5f);
				if (min_x < 0) min_x = 0;
				if (max_x > Graphics::screen_x) max_x = Graphics::screen_x;
				if (min_y < 0) min_y = 0;
				if (max_y > Graphics::screen_y) max_y = Graphics::screen_y;
				glScissor(min_x, min_y, max_x - min_x, max_y - min_y);
			}
			glEnable(GL_SCISSOR_TEST);
			(*i)->draw(int_view, screen_view, (*i)->scale * scale, recurse);
			glDisable(GL_SCISSOR_TEST);
		} //while (i != items.end())
	}
	//draw scribbles:
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glBoxToBox(viewport, screen_viewport);

	{ //enable clipping:
		int min_x = int((0.5f / Graphics::aspectf() * screen_viewport.min.x + 0.5f) * Graphics::screen_x + 0.5f);
		int max_x = int((0.5f / Graphics::aspectf() * screen_viewport.max.x + 0.5f) * Graphics::screen_x + 0.5f);
		int min_y = int((0.5f * screen_viewport.min.y + 0.5f) * Graphics::screen_y + 0.5f);
		int max_y = int((0.5f * screen_viewport.max.y + 0.5f) * Graphics::screen_y + 0.5f);
		if (min_x < 0) min_x = 0;
		if (max_x > Graphics::screen_x) max_x = Graphics::screen_x;
		if (min_y < 0) min_y = 0;
		if (max_y > Graphics::screen_y) max_y = Graphics::screen_y;
		glScissor(min_x, min_y, max_x - min_x, max_y - min_y);
	}
	glEnable(GL_SCISSOR_TEST);

	//draw arrows:
	if (tween_in != 0.0f) {
		float amt = tween_in * 10.0f;
		if (amt > 1.0f) amt = 1.0f;
		for (unsigned pass = 0; pass < 2; ++pass) {
			Vector2f offset = screen_height * make_vector(0.003f,-0.003f);
			if (pass == 0) {
				glLineWidth(2.5f);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f * amt);
			} else {
				glLineWidth(1.0f);
				glColor4f(1.0f, 1.0f, 1.0f, amt);
				offset = make_vector(0.0f, 0.0f);
			}
			glPushMatrix();
			glTranslatef(offset.x, offset.y, 0.0f);
			for (auto c = connections.begin(); c != connections.end(); ++c) {
				Vector2f from = make_vector(0.0f, 0.0f);
				PortDrawFn from_draw = NULL;
				Vector2f to = make_vector(0.0f, 0.0f);
				PortDrawFn to_draw = NULL;
				if (c->have_start) from = c->start;
				if (c->from_module && c->from_module->out_ports.size() > c->from_port) {
					if (!c->have_start) {
						c->start = c->from_module->out_ports[c->from_port]->position;
						from = c->start;
					}
					from = from * c->from_module->scale + c->from_module->position;
					from_draw = c->from_module->out_ports[c->from_port]->draw_fn;
				}
				if (c->have_end) to = c->end;
				if (c->to_module && c->to_module->in_ports.size() > c->to_port) {
					if (!c->have_end) {
						c->end = c->to_module->in_ports[c->to_port]->position;
						to = c->end;
					}
					to = to * c->to_module->scale + c->to_module->position;
					to_draw = c->to_module->in_ports[c->to_port]->draw_fn;
				}
				if (pass == 1 && from_draw) {
					from_draw(c->from_module->out_ports[c->from_port], *c, c->from_module->out_ports[c->from_port]->draw_fn_data);
				}
				if (pass == 1 && to_draw) {
					to_draw(c->to_module->in_ports[c->to_port], *c, c->to_module->in_ports[c->to_port]->draw_fn_data);
				}
				if ((!from_draw && !to_draw) || edit_mode) {
					if (pass == 0) {
						glColor4f(0.0f, 0.0f, 0.0f, 0.5f * amt);
					} else {
						glColor4f(1.0f, 1.0f, 1.0f, amt);
					}
					glBegin(GL_LINES);
					for (vector< Vector2f >::iterator p = c->points.begin(); p != c->points.end(); ++p) {
						glVertex(from);
						glVertex(*p);
						from = *p;
					}
					glVertex(from);
					glVertex(to);
					Vector2f norm = normalize(from - to);
					Vector2f perp = perpendicular(norm);
					glVertex(to);
					glVertex(to - perp * 0.05f + norm * 0.07f);
					glVertex(to);
					glVertex(to + perp * 0.05f + norm * 0.07f);
					glEnd();
				}
			}
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
		} //for (pass)
	} //end arrow drawing.

	//draw boxes:
	if (box_fade > 0.0f) {
		Box2f draw_inner = box;
		draw_inner.min = max(draw_inner.min, draw_area.min);
		draw_inner.max = min(draw_inner.max, draw_area.max);
		glBegin(GL_QUAD_STRIP);
		glColor4f(0.1f, 0.1f, 0.1f, box_fade * 0.5f);
		glVertex2f(draw_inner.min.x,draw_inner.min.y);
		glVertex2f( draw_area.min.x, draw_area.min.y);
		glVertex2f(draw_inner.min.x,draw_inner.max.y);
		glVertex2f( draw_area.min.x, draw_area.max.y);
		glVertex2f(draw_inner.max.x,draw_inner.max.y);
		glVertex2f( draw_area.max.x, draw_area.max.y);
		glVertex2f(draw_inner.max.x,draw_inner.min.y);
		glVertex2f( draw_area.max.x, draw_area.min.y);
		glVertex2f(draw_inner.min.x,draw_inner.min.y);
		glVertex2f( draw_area.min.x, draw_area.min.y);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 1.0f, box_fade);
		glVertex2f(box.min.x, box.min.y);
		glVertex2f(box.min.x, box.max.y);
		glVertex2f(box.max.x, box.max.y);
		glVertex2f(box.max.x, box.min.y);
		glEnd();
	}

	//draw scribbles:
	for (auto scrib = scribbles.begin(); scrib != scribbles.end(); ++scrib) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &((*scrib)[0]));
		glTranslatef(0.002f * screen_height, -0.002f * screen_height, 0.0f);
		glColor4f(0.0f, 0.0f, 0.0f,0.24f);
		glLineWidth(2.5f);
		glDrawArrays(GL_LINE_STRIP, 0, scrib->size());
		glTranslatef(-0.002f * screen_height, 0.002f * screen_height, 0.0f);
		glLineWidth(1.0f);
		glColor3f(1.0f, 1.0f, 1.0f);
		glDrawArrays(GL_LINE_STRIP, 0, scrib->size());
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	glDisable(GL_SCISSOR_TEST);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);


	Graphics::gl_errors("Frame draw");
}

void Frame::update_modules(float elapsed_time) {
	//Any input ports that aren't connected get default values:
	{
		map< Module *, set< unsigned int > > connected;
		for (auto i = modules.begin(); i != modules.end(); ++i) {
			connected.insert(make_pair(*i, set< unsigned int >()));
		}
		for (auto c = connections.begin(); c != connections.end(); ++c) {
			connected[c->to_module].insert(c->to_port);
		}
		for (auto i = modules.begin(); i != modules.end(); ++i) {
			for (unsigned int p = 0; p < (*i)->in_ports.size(); ++p) {
				if (!connected[*i].count(p)) {
					(*i)->in_ports[p]->reset();
				}
			}
		}
	}

	map< Module *, vector< Connection * > > outgoing;
	map< Module *, unsigned int > incoming;

	for (auto i = modules.begin(); i != modules.end(); ++i) {
		assert(!outgoing.count(*i));
		assert(!incoming.count(*i));
		outgoing.insert(make_pair(*i, vector< Connection * >()));
		incoming.insert(make_pair(*i, 0));
	}

	for (auto c = connections.begin(); c != connections.end(); ++c) {
		assert(outgoing.count(c->from_module));
		outgoing[c->from_module].push_back(&(*c));
		assert(incoming.count(c->to_module));
		incoming[c->to_module] += 1;
	}

	vector< Module * > done;
	for (auto i = incoming.begin(); i != incoming.end();) {
		if (i->second == 0) {
			done.push_back(i->first);
			auto old = i;
			++i;
			incoming.erase(old);
		} else {
			++i;
		}
	}
	while (1) {
		Module *cur = NULL;
		if (!done.empty()) {
			cur = done.back();
			done.pop_back();
		} else {
			unsigned int inc = (unsigned)-1;
			for (auto i = incoming.begin(); i != incoming.end(); ++i) {
				if (i->second < inc) {
					cur = i->first;
					inc = i->second;
				}
			}
			if (cur) {
				incoming.erase(incoming.find(cur));
			} else {
				return;
			}
		}
		cur->update(elapsed_time);
		assert(outgoing.count(cur));

		vector< Connection * > const &cons = outgoing[cur];
		for (auto c = cons.begin(); c != cons.end(); ++c) {
			assert((*c)->from_module == cur);
			assert((*c)->from_port < cur->out_ports.size());
			Module *to = (*c)->to_module;
			assert((*c)->to_port < to->in_ports.size());
			//actually transfer data:
			*(to->in_ports[(*c)->to_port]) = cur->out_ports[(*c)->from_port];
			auto found = incoming.find(to);
			if (found != incoming.end()) {
				assert(found->second != 0);
				--found->second;
				if (found->second == 0) {
					done.push_back(found->first);
					incoming.erase(found);
				}
			}
		}
	}
}

void Frame::set_target(Box2f viewport, Box2f screen_viewport, Vector2f mouse) {
	mouse = get_mouse(viewport, screen_viewport, mouse);

	//now mouse is in 'internal' coordinates!
	tween_target = NULL;
	for (auto i = items.begin(); i != items.end(); ++i) {
		Box2f span;
		span.min = (*i)->position - (*i)->size() * (*i)->scale * 0.5f;
		span.max = (*i)->position + (*i)->size() * (*i)->scale * 0.5f;
		if (span.contains(mouse)) {
			tween_target = *i;
		}
	}
}

Vector2f Frame::get_mouse(Box2f viewport, Box2f screen_viewport, Vector2f mouse) {
	Box2f internal_box, external_box;
	get_boxes(internal_box, external_box);

	mouse.x = (mouse.x - screen_viewport.min.x) * (viewport.max.x - viewport.min.x) / (screen_viewport.max.x - screen_viewport.min.x) + viewport.min.x;
	mouse.x = (mouse.x - external_box.min.x) * (internal_box.max.x - internal_box.min.x) / (external_box.max.x - external_box.min.x) + internal_box.min.x;
	mouse.y = (mouse.y - screen_viewport.min.y) * (viewport.max.y - viewport.min.y) / (screen_viewport.max.y - screen_viewport.min.y) + viewport.min.y;
	mouse.y = (mouse.y - external_box.min.y) * (internal_box.max.y - internal_box.min.y) / (external_box.max.y - external_box.min.y) + internal_box.min.y;

	return mouse;
}


void Frame::get_boxes(Box2f &internal_box, Box2f &external_box) {
	Vector2f internal_center = preview.center();
	Vector2f internal_size = preview.size();
	if (tween_in != 0) {
		Vector2f camera_center = screen_center;
		Vector2f camera_size = make_vector(screen_height * Graphics::aspectf(), screen_height);
		if (tween != 0 && tween_target) {
			camera_center += ease_pos(tween) * (tween_target->position - camera_center);
			camera_size += ease_size(tween) * (tween_target->scale * tween_target->size() - camera_size);
		}
		internal_center += ease_pos(tween_in) * (camera_center - internal_center);
		internal_size += ease_size(tween_in) * (camera_size - internal_size);
	}
	
	{
		Vector2f external_size = size();
		external_box.min = -0.5f * external_size;
		external_box.max = 0.5f * external_size;
		//aspect ratio correction:
		if (internal_size.x * external_size.y < external_size.x * internal_size.y) {
			internal_size.x = external_size.x * internal_size.y / external_size.y;
		} else {
			internal_size.y = external_size.y * internal_size.x / external_size.x;
		}
		internal_box.min = internal_center - 0.5f * internal_size;
		internal_box.max = internal_center + 0.5f * internal_size;
	}
}

void Frame::correct_camera() {
	Box2f valid_box = area;
	valid_box.min.x += 0.5f * screen_height * Graphics::aspectf();
	valid_box.max.x -= 0.5f * screen_height * Graphics::aspectf();
	valid_box.min.y += 0.5f * screen_height;
	valid_box.max.y -= 0.5f * screen_height;
	if (valid_box.min.x > valid_box.max.x) {
		valid_box.min.x = valid_box.max.x = 0.5f * (valid_box.min.x + valid_box.max.x);
	}
	if (valid_box.min.y > valid_box.max.y) {
		valid_box.min.y = valid_box.max.y = 0.5f * (valid_box.min.y + valid_box.max.y);
	}
	screen_center = valid_box.clamp(screen_center);
}
