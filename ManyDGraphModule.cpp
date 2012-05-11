#include "ManyDGraphModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

using std::cout;
using std::ifstream;
using std::istringstream;
using std::getline;

namespace {
	Module *create(const std::string &params) {
		return new ManyDGraphModule(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("manydgraph", create, "infile.csv outfile.csv");
		}
	} fred;
}

void read_csv(std::string from, vector< float > &into, unsigned int &cols) {
	cols = 0;
	into.clear();
	ifstream file(from.c_str());
	string str;
	while (getline(file, str)) {
		unsigned int count = 0;
		istringstream line(str);
		float val;
		while (line >> val) {
			into.push_back(val);
			++count;
			char temp;
			if (!(line >> temp)) break;
			if (temp != ',') {
				cerr << "Expecting ','" << endl;
				break;
			}
		}
		if (cols == 0) {
			cols = count;
		}
		if (cols != count) {
			cerr << "Bad count in row." << endl;
			break;
		}
	}
	cerr << "Read " << into.size() << " values in " << cols << " columns.";
	if (cols != 0) {
		cout << " (That's " << into.size() / cols << " rows.)";
	}
	cout << endl;
}

ManyDGraphModule::ManyDGraphModule(std::string params) {
	for (unsigned int i = 0; i < params.size(); ++i) {
		if (params[i] == ' ') {
			read_csv(params.substr(0,i), in_data, in_size);
			if (in_size == 0) {
				cerr << "WARNING: no in data." << endl;
				in_data.clear();
			}

			unsigned int out_size = 0;
			vector< float > out_temp;
			read_csv(params.substr(i+1), out_temp, out_size);
			if (out_size > 3) {
				cerr << "WARNING: too much out data." << endl;
			}
			out_data.resize(in_data.size() / in_size, make_vector(1.0f, 1.0f, 1.0f));
			if (out_size * in_data.size() / in_size != out_temp.size()) {
				cerr << "WARNING: out data and in data don't match in size." << endl;
			}
			Vector3f out_min = make_vector(0.0f, 0.0f, 0.0f);
			Vector3f out_max = make_vector(0.0f, 0.0f, 0.0f);
			for (unsigned int i = 0; i < out_temp.size(); i += out_size) {
				assert(out_size > 0);
				out_data[i/out_size].x = out_temp[i];
				if (out_size > 1) {
					out_data[i/out_size].y = out_temp[i+1];
				}
				if (out_size > 2) {
					out_data[i/out_size].z = out_temp[i+2];
				}
				if (i == 0) out_min = out_max = out_data[i/out_size];
				else {
					out_min = min(out_min, out_data[i/out_size]);
					out_max = max(out_max, out_data[i/out_size]);
				}
			}
			Vector3f out_mul = make_vector(0.0f, 0.0f, 0.0f);
			for (unsigned int c = 0; c < 3; ++c) {
				if (out_max[c] - out_min[c] > 0.0f) {
					out_mul[c] = 1.0f / (out_max[c] - out_min[c]);
				}
			}
			for (unsigned int i = 0; i < out_data.size(); ++i) {
				out_data[i] = product(out_data[i] - out_min, out_mul);
			}
			break;
		}
	}
	axis.resize(in_size, make_vector(0.0f, 0.0f, 0.0f, 0.0f));
	for (unsigned int i = 0; i < in_size && i < 3; ++i) {
		axis[i][i] = 1.0f;
		axis[i].w = 1.0f;
	}
	current_axis = 0;
	vector< Vector2f > bounds(in_size);
	for (unsigned int b = 0; b < in_data.size(); b += in_size) {
		for (unsigned int i = 0; i < in_size; ++i) {
			if (b == 0 || bounds[i].x > in_data[b+i]) {
				bounds[i].x = in_data[b+i];
			}
			if (b == 0 || bounds[i].y < in_data[b+i]) {
				bounds[i].y = in_data[b+i];
			}
		}
	}
	for (unsigned int i = 0; i < in_size; ++i) {
		if (bounds[i].x >= bounds[i].y) {
			//no variation in this dim.
			bounds[i].y = 1.0f;
		} else {
			bounds[i] = make_vector(-0.5f * (bounds[i].x + bounds[i].y), 2.0f / (bounds[i].y - bounds[i].x));
		}
	}
	for (unsigned int b = 0; b < in_data.size(); b += in_size) {
		for (unsigned int i = 0; i < in_size; ++i) {
			in_data[b+i] += bounds[i].x;
			in_data[b+i] *= bounds[i].y;
		}
	}
}

Vector2f ManyDGraphModule::size() {
	return make_vector(2.5f, 2.5f);
}

void ManyDGraphModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glScalef(1.0f, 1.0f, 0.2f); //make sure our graph manages to fit in the z range available... well, maybe.

	Vector2f sz = size();

	if (in_size != 0) {
		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (unsigned int a = 0; a < axis.size(); ++a) {
			if (axis[a].w != 0.0f) {
				glVertex3f(0.0f, 0.0f, 0.0f);
				glVertex(axis[a].xyz);
			}
		}
		glEnd();
		glEnable(GL_DEPTH_TEST);
		glPointSize(2.0f);
		glBegin(GL_POINTS);
		unsigned int count = in_data.size() / in_size;
		for (unsigned int p = 0; p < count; ++p) {
			Vector3f acc = make_vector(0.0f, 0.0f, 0.0f);
			for (unsigned int i = 0; i < in_size; ++i) {
				acc += axis[i].xyz * in_data[p*in_size+i];
			}
			glColor(out_data[p]);
			glVertex(acc);
		}
		glEnd();
		glDisable(GL_DEPTH_TEST);
	}

	glBegin(GL_LINE_LOOP);
	glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
	glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
	glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("ManyDGraph::draw");

}

void ManyDGraphModule::update(float elapsed_time) {
	if (goal_axis.size() == axis.size()) {
		if (elapsed_time >= goal_time) {
			axis = goal_axis;
			goal_axis.clear();
		} else {
			float amt = elapsed_time / goal_time;
			goal_time -= elapsed_time;
			for (unsigned int a = 0; a < axis.size(); ++a) {
				axis[a] += (goal_axis[a] - axis[a]) * amt;
				axis[a].xyz = normalize(axis[a].xyz) * axis[a].w;
			}
		}
	}
}

void ManyDGraphModule::update_current_axis(Vector2f local_mouse) {
	if (current_axis < axis.size()) {
		axis[current_axis].xy = local_mouse;
		axis[current_axis].z = 0.0f;
		float len = length(axis[current_axis].xy);
		if (len > 1.0f) {
			axis[current_axis].xyz /= len;
			axis[current_axis].w = 1.0f;
		} else {
			axis[current_axis].w = len;
		}
	}
}

bool ManyDGraphModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_RIGHT) {
			update_current_axis(local_mouse);
			return true;
		}
		if (event.button.button == SDL_BUTTON_LEFT) {
			return true;
		}
	}
	if (event.type == SDL_MOUSEMOTION) {
		if (event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
			update_current_axis(local_mouse);
			return true;
		} else if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			{
				float rx = event.motion.xrel * 0.01f;
				Vector3f new_x = make_vector(cosf(rx), 0.0f, sinf(rx));
				Vector3f new_z = make_vector(-sinf(rx), 0.0f, cosf(rx));
				for (unsigned int a = 0; a < axis.size(); ++a) {
					axis[a].xyz = normalize(axis[a].x * new_x + axis[a].z * new_z + make_vector(0.0f, axis[a].y, 0.0f)) * axis[a].w;
				}
			}
			{
				float ry = event.motion.yrel *-0.01f;
				Vector3f new_y = make_vector(0.0f, cosf(ry), sinf(ry));
				Vector3f new_z = make_vector(0.0f,-sinf(ry), cosf(ry));
				for (unsigned int a = 0; a < axis.size(); ++a) {
					axis[a].xyz = normalize(axis[a].z * new_z + axis[a].y * new_y + make_vector(axis[a].x, 0.0f, 0.0f)) * axis[a].w;
				}
			}
			return true;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (event.button.button == SDL_BUTTON_RIGHT) {
			return true;
		}
		if (event.button.button == SDL_BUTTON_LEFT) {
			return true;
		}
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		goal_axis.clear();
		goal_axis.resize(in_size, make_vector(0.0f, 0.0f, 0.0f, 0.0f));
		for (unsigned int i = 0; i < in_size && i < 3; ++i) {
			goal_axis[i][i] = 1.0f;
			goal_axis[i].w = 1.0f;
		}
		goal_time = 1.0f;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
		current_axis = event.key.keysym.sym - SDLK_1;
	}
	return false;
}
