#include "BL_csv.hpp"

#include <Basis/Error.hpp>
#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <cassert>

#include <fstream>
#include <sstream>

using std::ifstream;
using std::istringstream;
using std::getline;

namespace {
	Module *create_module(const std::string &params_in) {
		return new CSVMotion(params_in, 1.0 / 60.0);
	}
	class Fred {
	public:
		Fred() {
			register_module("bl_csv", create_module, " <file.csv>");
		}
	} fred;
}

template< typename T >
void read_vector(string line, vector< T > &into) {
	into.clear();
	line += ',';
	unsigned int start = 0;
	for (unsigned int i = 0; i < line.size(); ++i) {
		if (line[i] == ',') {
			istringstream str(line.substr(start, i - 1 - start));
			T temp;
			if (str >> temp) {
				into.push_back(temp);
			}
			start = i + 1;
		}
	}
}

CSVMotion::CSVMotion(std::string filename, double _seconds_per_frame) : playing(false), time_acc(0.0), seconds_per_frame(_seconds_per_frame) {
	out_ports.push_back(&lengths);
	lengths.name = "lengths";
	lengths.position = make_vector(0.5, 0.0);

	out_ports.push_back(&frame);
	frame.name = "frame";
	frame.position = make_vector(0.5, 0.4);
	frame() = 0;

	in_ports.push_back(&highlight);
	highlight.name = "highlight";
	highlight.position = make_vector(-0.5, 0.0);

	{
		ifstream file(filename.c_str());
		string line;
		while (getline(file, line)) {
			if (names.empty()) {
				read_vector(line, names);
				if (names.size() % 3 != 0) {
					LOG_WARNING("csv doesn't have / 3 names; will have weird points.");
				}
			} else {
				vector< double > temp;
				read_vector(line, temp);
				if (temp.size() != names.size()) {
					LOG_WARNING("Skipping csv line; bad size.");
				} else {
					positions.push_back(vector< Vector3d >());
					for (unsigned i = 0; i + 2 < temp.size(); i += 3) {
						positions.back().push_back(make_vector(temp[i], temp[i+1], temp[i+2]));
					}
				}
			}
		}
		LOG_INFO("Read " << positions.size() << " frames.");
	}

	{
		bounds.min = make_vector(-0.5, -0.5, 0.0);
		bounds.max = make_vector( 0.5,  0.5, 1.0);
		bool have_bounds = false;
		roots.resize(positions.size(), make_vector(0.0, 0.0, 0.0));
		for (vector< vector< Vector3d > >::iterator pos = positions.begin(); pos != positions.end(); ++pos) {
			//find center of mass on floor and subtract:
			Vector2d acc = make_vector(0.0, 0.0);
			unsigned int count = 0;
			for (vector< Vector3d >::iterator p = pos->begin(); p != pos->end(); ++p) {
				if (*p != make_vector(0.0, 0.0, 0.0)) {
					acc += p->xy;
					++count;
				}
			}

			Vector3d &root = roots[pos - positions.begin()];

			if (count) {
				root.xy = acc / count;
			}

			//TODO: root rotation?

			for (vector< Vector3d >::iterator p = pos->begin(); p != pos->end(); ++p) {
				if (*p == make_vector(0.0, 0.0, 0.0)) continue;
				p->xy -= root.xy;
				Vector2d x_dir = make_vector(cos(root.z), sin(root.z));
				Vector2d y_dir = perpendicular(x_dir);
				p->xy = make_vector(p->xy * x_dir, p->xy * y_dir);
			}
			
			//adjust bounds properly:
			for (vector< Vector3d >::iterator p = pos->begin(); p != pos->end(); ++p) {
				if (*p == make_vector(0.0, 0.0, 0.0)) continue;
				if (!have_bounds) {
					bounds.min = bounds.max = *p;
					have_bounds = true;
				} else {
					bounds.min = min(bounds.min, *p);
					bounds.max = max(bounds.max, *p);
				}
			}
		}
	}
	
	if (seconds_per_frame <= 0.0) {
		seconds_per_frame = 1.0 / 120.0;
	}

	update_lengths();
}

CSVMotion::~CSVMotion() {
}

Vector2f CSVMotion::size() {
	return make_vector(1.0f, 1.5f);
}

namespace {
	unsigned int mod_pow(unsigned int base, unsigned int exp, unsigned int mod) {
		unsigned int out = 1;
		while (exp) {
			if (exp & 1) {
				out = (out * base) % mod;
			}
			exp >>= 1;
			base = (base * base) % mod;
		}
		return out;
	}
}

Vector3f id_color(unsigned int id) {
	return normalize(make_vector< float >(
		mod_pow(733, id, 1987) / 1986.0f,
		mod_pow(1237, id, 1583) / 1582.0f,
		mod_pow(1753, id, 2357) / 2356.0f));
}

void CSVMotion::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	if (frame() < positions.size()) {
		glPushMatrix();
		{
			double radius = length(bounds.size().xy * 0.5);
			const double Skew = 0.5;
			const double up_fac = cos(Skew);
			const double in_fac = sin(Skew);
			double scale = 1.0;
			if (scale * radius * 2 > size().x) {
				scale = size().x / (radius * 2);
			}
			if (scale * (radius * 2 * in_fac + bounds.size().z * up_fac) > size().y) {
				scale = size().y / (radius * 2 * in_fac + bounds.size().z * up_fac);
			}
			static double rot = 1.0; //DEBUG
			Vector3d z_dir = scale * make_vector(0.0,cos(Skew),-sin(Skew));
			Vector2d in_dir = make_vector(sin(Skew), cos(Skew));
			Vector3d x_dir = scale * make_vector(cos(rot), sin(rot) * in_dir);
			Vector3d y_dir = scale * make_vector(-sin(rot), cos(rot) * in_dir);
			double z_scale = 0.0; //DEBUG
			double mat[16] = {
				x_dir.x, x_dir.y, x_dir.z * z_scale, 0,
				y_dir.x, y_dir.y, y_dir.z * z_scale, 0,
				z_dir.x, z_dir.y, z_dir.z * z_scale, 0,
				0, 0, 0, 1
			};
			glMultMatrixd(mat);
			glTranslatef(-bounds.center().x, -bounds.center().y, -bounds.center().z);
		}

		//draw grid underfoot.
		{
			const double GridSize = 500.0;
			Vector3d root = roots[frame()];
			Vector2d grid_pos = make_vector(fmod(-root.x, GridSize), fmod(-root.y, GridSize));
			double radius = length(bounds.size().xy * 0.5);
			while (grid_pos.x > bounds.center().x - radius) grid_pos.x -= GridSize;
			while (grid_pos.y > bounds.center().y - radius) grid_pos.y -= GridSize;
			glBegin(GL_LINES);
			while (grid_pos.x < bounds.center().x + radius) {
				double height = radius * radius - pow(grid_pos.x - bounds.center().x, 2.0);
				if (height > 0.0) {
					height = sqrt(height);
					glColor3f(0.6, 0.5, 0.5);
					glVertex3f(grid_pos.x, bounds.center().y + height, bounds.min.z);
					glVertex3f(grid_pos.x, bounds.center().y - height, bounds.min.z);
				}
				grid_pos.x += GridSize;
			}
			while (grid_pos.y < bounds.center().y + radius) {
				double height = radius * radius - pow(grid_pos.y - bounds.center().y, 2.0);
				if (height > 0.0) {
					height = sqrt(height);
					glColor3f(0.5, 0.6, 0.5);
					glVertex3f(bounds.center().x + height, grid_pos.y, bounds.min.z);
					glVertex3f(bounds.center().x - height, grid_pos.y, bounds.min.z);
				}
				grid_pos.y += GridSize;
			}
			glEnd();
		}

		vector< Vector3d > const &pos = positions[frame()];

		//highlighted distances:
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		for (unsigned int h = 0; h < highlight().size(); ++h) {
			unsigned int v = highlight()[h];
			unsigned int a = 1;
			unsigned int b = 0;
			while (v > a) {
				v -= a;
				a += 1;
			}
			b = v;
			if (a < pos.size() && b < pos.size()) {
				glColor(id_color(highlight()[h]));
				glVertex3f(pos[a].x, pos[a].y, pos[a].z);
				glVertex3f(pos[b].x, pos[b].y, pos[b].z);
			}
		}
		glEnd();
		glLineWidth(1.0f);

		//marker points:
		glPointSize(3.0f);
		glEnable(GL_POINT_SMOOTH);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (vector< Vector3d >::const_iterator p = pos.begin(); p != pos.end(); ++p) {
			glVertex3f(p->x, p->y, p->z);
		}
		glEnd();
		glDisable(GL_POINT_SMOOTH);
		
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

void CSVMotion::update(float elapsed_time) {
	if (playing) {
		time_acc += elapsed_time;
		bool need_update = false;
		while (time_acc > 0) {
			time_acc -= seconds_per_frame;
			if (frame() + 1 < positions.size()) {
				frame() += 1;
				need_update = true;
			} else {
				playing = false;
			}
		}
		if (need_update) {
			update_lengths();
		}
	}
}

bool CSVMotion::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		if (playing) {
			playing = false;
		} else {
			playing = true;
			if (frame() + 1 >= positions.size()) {
				frame() = 0;
			}
		}
		return true;
	}
	return false;
}

void CSVMotion::update_lengths() {
	lengths().clear();
	if (frame() >= positions.size()) {
		return;
	}
	vector< Vector3d > const &pos = positions[frame()];
	for (unsigned int a = 0; a < pos.size(); ++a) {
		for (unsigned int b = 0; b < a; ++b) {
			lengths().push_back(length(pos[b] - pos[a]));
		}
	}
}
