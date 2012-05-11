#include "BL_lengths.hpp"

#include <Basis/Error.hpp>
#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <cassert>

#include <utility>
#include <algorithm>

using std::pair;
using std::sort;


namespace {
	Module *create_module(const std::string &params_in) {
		return new Lengths();
	}
	class Fred {
	public:
		Fred() {
			register_module("bl_lengths", create_module, "");
		}
	} fred;
}

Lengths::Lengths() : prev_frame(-1U), have_cursor(false), cursor_x(0.0) {
	in_ports.push_back(&lengths);
	lengths.name = "lengths";
	lengths.position = make_vector(-1.0, 0.0);

	in_ports.push_back(&frame);
	frame.name = "frame";
	frame.position = make_vector(-1.0, 0.2);
	frame() = 0;

	out_ports.push_back(&highlight);
	highlight.name = "highlight";
	highlight.position = make_vector(1.0, 0.0);

}

Lengths::~Lengths() {
}

Vector2f Lengths::size() {
	return make_vector(2.0f, 1.5f);
}

double dis_scale(double in) {
	if (in >= 0.0) {
		return  log(in + 1.0);
	} else {
		return -log(-in + 1.0);
	}
}

extern Vector3f id_color(unsigned int id);

void Lengths::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor3f(0.02f, 0.02f, 0.02f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	if (lengths().size() == mins.size() && lengths().size() == maxes.size() && lengths().size() == order.size() && lengths().size() + 1 == splits.size() && !lengths().empty()) {
		glPushMatrix();
		double diff = 2.0 * dis_scale(0.5 * (maxes[order.back()] - mins[order.back()]));
		if (diff < 0.001) diff = 0.001;
		glScalef(1.0, size().y / diff, 1.0);
		glBegin(GL_QUADS);
		vector< unsigned int > order_pos(order.size(), 0);
		for (unsigned int i = 0; i < order.size(); ++i) {
			assert(order[i] < order_pos.size());
			order_pos[order[i]] = i;
		}
		for (unsigned int s = 0; s < highlight().size(); ++s) {
			if (highlight()[s] >= order.size()) continue;
			unsigned int i = order_pos[highlight()[s]];
			glColor(id_color(highlight()[s]));
			if (i >= lengths().size()) continue; //problem!
			double min = mins[order[i]];
			double max = maxes[order[i]];
			double c = (min + max) / 2.0;
			double minx = splits[i];
			double maxx = splits[i+1];
			double w = maxx - minx;
			minx += 0.1 * w;
			maxx -= 0.1 * w;
			glVertex2f(minx, dis_scale(min - c));
			glVertex2f(maxx, dis_scale(min - c));
			glVertex2f(maxx, dis_scale(max - c));
			glVertex2f(minx, dis_scale(max - c));
		}
		glEnd();

		glBegin(GL_LINES);
		for (unsigned int i = 0; i < order.size(); ++i) {
			double min = mins[order[i]];
			double max = maxes[order[i]];
			double c = (min + max) / 2.0;
			double minx = splits[i];
			double maxx = splits[i+1];
			double w = maxx - minx;
			minx += 0.1 * w;
			maxx -= 0.1 * w;
			glColor3f(1.0f, 1.0f, 0.4f);
			glVertex2f(minx, dis_scale(lengths()[order[i]]- c));
			glVertex2f(maxx, dis_scale(lengths()[order[i]]- c));
		}
		glEnd();
		glBegin(GL_LINES);
		for (unsigned int i = 0; i < order.size(); ++i) {
			double min = mins[order[i]];
			double max = maxes[order[i]];
			double c = (min + max) / 2.0;
			double minx = splits[i];
			double maxx = splits[i+1];
			double w = maxx - minx;
			minx += 0.1 * w;
			maxx -= 0.1 * w;
			glColor4f(0.7f, 0.7f, 0.7f, 0.2f);
			glVertex2f(minx, dis_scale(min - c));
			glVertex2f(maxx, dis_scale(min - c));
			glVertex2f(maxx, dis_scale(min - c));
			glVertex2f(maxx, dis_scale(max - c));
			glVertex2f(maxx, dis_scale(max - c));
			glVertex2f(minx, dis_scale(max - c));
			glVertex2f(minx, dis_scale(max - c));
			glVertex2f(minx, dis_scale(min - c));
		}
		glEnd();

		glBegin(GL_LINES);
		vector< double > ticks;
		vector< Vector4f > colors;
		for (unsigned int i = 1; i < 10; ++i) {
			ticks.push_back(i);
			if (i == 1) colors.push_back(make_vector(0.7f, 0.2f, 0.5f, 0.6f));
			else colors.push_back(make_vector(0.6f, 0.2f, 0.7f, 0.3f));
		}
		for (unsigned int i = 1; i < 10; ++i) {
			ticks.push_back(i*10);
			if (i == 1) colors.push_back(make_vector(1.0f, 0.6f, 1.0f, 0.8f));
			else colors.push_back(make_vector(0.6f, 0.2f, 0.7f, 0.3f));
		}
		for (unsigned int i = 1; i < 10; ++i) {
			ticks.push_back(i*100);
			if (i == 1) colors.push_back(make_vector(0.7f, 0.2f, 0.5f, 0.6f));
			else colors.push_back(make_vector(0.6f, 0.2f, 0.7f, 0.3f));
		}
		for (unsigned int i = 1; i < 10; ++i) {
			ticks.push_back(i*1000);
			if (i == 1) colors.push_back(make_vector(0.7f, 0.2f, 0.5f, 0.6f));
			else colors.push_back(make_vector(0.6f, 0.2f, 0.7f, 0.3f));
		}
		assert(colors.size() == ticks.size());
		for (unsigned int t = 0; t < ticks.size(); ++t) {
			if ((maxes[order.back()] - mins[order.back()]) * 0.5 > ticks[t]) {
				glColor(colors[t]);
				glVertex2f(-0.5 * size().x, dis_scale(ticks[t]));
				glVertex2f( 0.5 * size().x, dis_scale(ticks[t]));
				glVertex2f(-0.5 * size().x, dis_scale(-ticks[t]));
				glVertex2f( 0.5 * size().x, dis_scale(-ticks[t]));
			}
		}


		glEnd();

		glPopMatrix();
	}

	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}

float Lengths::cursor_alpha() {
	have_cursor = true;
	return 1.0f;
}

void Lengths::update(float elapsed_time) {
	if (frame() != prev_frame) {
		prev_frame = frame();
		if (mins.size() != lengths().size()) {
			mins = lengths();
		}
		if (maxes.size() != lengths().size()) {
			maxes = lengths();
		}
		vector< pair< double, unsigned int > > diffs(lengths().size());
		for (unsigned int i = 0; i < lengths().size(); ++i) {
			double l = lengths()[i];
			if (l < mins[i]) mins[i] = l;
			if (l > maxes[i]) maxes[i] = l;
			diffs[i].first = maxes[i] - mins[i];
			diffs[i].second = i;
		}
		sort(diffs.begin(), diffs.end());
		order.resize(lengths().size());
		for (unsigned int i = 0; i < lengths().size(); ++i) {
			order[i] = diffs[i].second;
		}
	}

	vector< double > wanted_splits(lengths().size() + 1);
	{
		vector< double > sizes(lengths().size());
		double sum = 0.0;
		vector< unsigned int > highlight_pos;
		{
			vector< unsigned int > pos(order.size());
			for (unsigned int i = 0; i < order.size(); ++i) {
				assert(order[i] < pos.size());
				pos[order[i]] = i;
			}
			for (unsigned int h = 0; h < highlight().size(); ++h) {
				if (highlight()[h] < pos.size()) {
					highlight_pos.push_back(pos[highlight()[h]]);
				}
			}
		}
		for (unsigned int i = 0; i < sizes.size(); ++i) {
			sizes[i] = 0.01;
			for (unsigned int h = 0; h < highlight_pos.size(); ++h) {
				sizes[i] += 1.0 / (1.0 + abs(highlight_pos[h] - i));
			}
			sum += sizes[i];
		}
		if (sum != 0.0) {
			sum = 1.0 / sum;
		}
		double acc = 0.0;
		for (unsigned int i = 0; i < splits.size(); ++i) {
			wanted_splits[i] = acc * sum * size().x - 0.5 * size().x;
			if (i < sizes.size()) {
				acc += sizes[i];
			}
		}
	}

	if (splits.size() != wanted_splits.size()) {
		splits = wanted_splits;
	} else {
		double fac = 1.0 - pow(0.5, elapsed_time * 20);
		for (unsigned int i = 0; i < splits.size(); ++i) {
			splits[i] += fac * (wanted_splits[i] - splits[i]);
		}
	}

	have_cursor = false;
}

bool Lengths::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	cursor_x = local_mouse.x;
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		if (SDL_GetModState() & KMOD_SHIFT) {
			if (mins.size() == maxes.size()) {
				highlight().clear();
				for (unsigned int i = 0; i < mins.size(); ++i) {
					if (maxes[i] - mins[i] < 30) {
						highlight().push_back(i);
					}
				}
			}
		} else {
			if (highlight().empty()) {
				if (!order.empty()) {
					highlight().push_back(order[0]);
				}
			} else {
				highlight().clear();
			}
		}
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		if (splits.size() == lengths().size()+1) {
			unsigned int sel = -1U;
			for (unsigned int i = 0; i + 1 < splits.size(); ++i) {
				if (cursor_x >= splits[i] && cursor_x < splits[i+1]) {
					sel = i;
					break;
				}
			}
			if (sel < order.size()) {
				sel = order[sel];
				for (unsigned int h = 0; h < highlight().size(); ++h) {
					if (highlight()[h] == sel) {
						highlight().erase(highlight().begin() + h);
						sel = -1U;
					}
				}
				if (sel != -1U) {
					highlight().push_back(sel);
				}
			}
		}
		return true;
	}
	return false;
}
