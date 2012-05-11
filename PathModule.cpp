#include "PathModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>
#include <fstream>
#include <cassert>

using std::istringstream;
using std::ifstream;

using std::make_pair;

const unsigned int Window = 5;
const float FPS = 10.0f;

namespace {
	Module *create_module(const std::string &params) {
		PathModule *m = new PathModule();
		istringstream p(params);
		string fnA, fnB;
		p >> fnA >> fnB;
		ifstream fileA(fnA.c_str());
		{
			Vector2f temp;
			while (fileA >> temp) {
				m->path_a.push_back(temp);
			}
		}
		ifstream fileB(fnB.c_str());
		{
			Vector2f temp;
			while (fileB >> temp) {
				m->path_b.push_back(temp);
			}
		}
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("path", create_module, "fileA fileB");
		}
	} fred;
}

Vector2f PathModule::size() {
	return make_vector(3.0f, 2.0f);
}

void PathModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(-1.5f, -1.0f);
	glVertex2f( 1.5f, -1.0f);
	glVertex2f( 1.5f,  1.0f);
	glVertex2f(-1.5f,  1.0f);
	glEnd();


	glPushAttrib(GL_SCISSOR_BIT);

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


	const float Scales[2] = { 0.005f, 0.02f };
	//path for A:
	{
	Vector3f col = make_vector(0.9f, 0.3f, 0.3f);
	for (unsigned int i = 0; i < path_a.size(); ++i) {
		float s = Scales[(i % 10) == 0];
		Vector2f a = path_a[i] + s*make_vector( 0.0000000f, 1.000000f );
		Vector2f b = path_a[i] + s*make_vector( 0.9610565f, 0.309017f );
		Vector2f c = path_a[i] + s*make_vector( 0.5877853f,-0.809017f );
		Vector2f d = path_a[i] + s*make_vector(-0.5877853f,-0.809017f );
		Vector2f e = path_a[i] + s*make_vector(-0.9610565f, 0.309017f );
		glBegin(GL_TRIANGLES);
		glColor(col, 0.5f);
		glVertex(a);glVertex(b);glVertex(c);
		glVertex(a);glVertex(c);glVertex(d);
		glVertex(a);glVertex(d);glVertex(e);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor(col);
		glVertex(a); glVertex(b); glVertex(c); glVertex(d); glVertex(e);
		glEnd();
	}
	}

	//path for B:
	{
	Vector3f col = make_vector(0.3f, 0.9f, 0.3f);
	for (unsigned int i = 0; i < path_b.size(); ++i) {
		float s = Scales[(i % 10) == 0];
		Vector2f a = path_b[i] + s*make_vector( 1.0f, 0.0f );
		Vector2f b = path_b[i] + s*make_vector(-0.5f, 0.8660254f );
		Vector2f c = path_b[i] + s*make_vector(-0.5f,-0.8660254f );
		glBegin(GL_TRIANGLES);
		glColor(col, 0.5f);
		glVertex(a);glVertex(b);glVertex(c);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor(col);
		glVertex(a);glVertex(b);glVertex(c);
		glEnd();
	}
	}

	//path for C:
	if (show_c) {
	Vector3f col = make_vector(0.9f, 0.9f, 0.3f);
	for (unsigned int i = 0; i < path_c.size(); ++i) {
		float s = Scales[(i % 10) == 0];
		Vector2f a = path_c[i] + s*make_vector(-1.0f, 0.0f );
		Vector2f b = path_c[i] + s*make_vector( 0.5f, 0.8660254f );
		Vector2f c = path_c[i] + s*make_vector( 0.5f,-0.8660254f );
		glBegin(GL_TRIANGLES);
		glColor(col, 0.5f);
		glVertex(a);glVertex(b);glVertex(c);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor(col);
		glVertex(a);glVertex(b);glVertex(c);
		glEnd();
	}
	}

	//Begin and End points:
	{
		Vector2f begin = make_vector(0.0f, 0.0f);
		Vector2f end = make_vector(0.0f, 0.0f);
		if (!path_a.empty()) {
			if (cur_leave_a < 0) begin = path_a[0];
			else if (cur_leave_a >= path_a.size()) begin = path_a.back();
			else begin = path_a[cur_leave_a];
		}
		if (!path_b.empty()) {
			if (cur_join_b < 0) end = path_b[0];
			else if (cur_join_b >= path_b.size()) end = path_b.back();
			else end = path_b[cur_join_b];
		}
		const float s1 = 0.03f;
		const float s2 = 0.03f * sqrtf(2.0f);
		Vector3f c1 = make_vector(0.9f, 0.3f, 0.9f);
		Vector3f c2 = make_vector(0.3f, 0.9f, 0.9f);
		glBegin(GL_QUADS);
		glColor(c1, 0.9f);
		glVertex(begin + s1*make_vector(-1.0f, 1.0f));
		glVertex(begin + s1*make_vector(-1.0f,-1.0f));
		glVertex(begin + s1*make_vector( 1.0f,-1.0f));
		glVertex(begin + s1*make_vector( 1.0f, 1.0f));
		glColor(c2, 0.9f);
		glVertex(end + s2*make_vector(-1.0f, 0.0f));
		glVertex(end + s2*make_vector( 0.0f,-1.0f));
		glVertex(end + s2*make_vector( 1.0f, 0.0f));
		glVertex(end + s2*make_vector( 0.0f, 1.0f));
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor(c1);
		glVertex(begin + s1*make_vector(-1.0f, 1.0f));
		glVertex(begin + s1*make_vector(-1.0f,-1.0f));
		glVertex(begin + s1*make_vector( 1.0f,-1.0f));
		glVertex(begin + s1*make_vector( 1.0f, 1.0f));
		glEnd();
		glBegin(GL_LINE_LOOP);
		glColor(c2);
		glVertex(end + s2*make_vector(-1.0f, 0.0f));
		glVertex(end + s2*make_vector( 0.0f,-1.0f));
		glVertex(end + s2*make_vector( 1.0f, 0.0f));
		glVertex(end + s2*make_vector( 0.0f, 1.0f));
		glEnd();

		if (animate) {
			int frame = (int)(anim_time * FPS);
			float frac = anim_time * FPS - frame;
			Vector2f before = make_vector(0.0f, 0.0f);
			Vector2f after = make_vector(0.0f, 0.0f);
			if (!path_a.empty()) {
				if (frame < 0) before = path_a[0];
				else if ((unsigned)frame <= cur_leave_a) before = path_a[frame];
				if (frame + 1 < 0) after = path_a[0];
				else if ((unsigned)(frame+1) <= cur_leave_a) after = path_a[frame+1];
			}
			frame -= cur_leave_a + 1;
			frame += Window;
			bool have_c = false;
			if (!path_c.empty()) {
				if (frame >= (signed)Window && (unsigned)frame < path_c.size() - Window) {
					before = path_c[frame];
					have_c = true;
				}
				if (frame+1 >= (signed)Window && (unsigned)(frame+1) < path_c.size() - Window) {
					after = path_c[frame+1];
					have_c = true;
				}
			}
			frame -= Window + cur_intermed;
			frame += cur_join_b;
			if (!path_b.empty()) {
				if (frame >= (signed)cur_join_b && (unsigned)frame < path_b.size()) before = path_b[frame];
				else if (frame > (signed)path_b.size()) before = path_b.back();
				if (frame+1 >= (signed)cur_join_b && (unsigned)(frame+1) < path_b.size()) after = path_b[frame+1];
				else if (frame+1 > (signed)path_b.size()) after = path_b.back();
			}
			if (!have_c || show_c) {
				Vector2f at = before + (after - before) * frac;
				Vector3f col = make_vector(1.0f, 1.0f, 1.0f);
				glBegin(GL_TRIANGLE_FAN);
				const float s = 0.1f * sqrtf(2.0f);
				for (unsigned int a = 0; a < 17; ++a) {
					float ang = a / 17.0f * float(M_PI) * 2.0f;
					glColor(col * (0.5f + 0.3f * sinf(ang)));
					glVertex(at + s*make_vector(cosf(ang),sinf(ang)));
				}
				glEnd();
				glBegin(GL_LINE_LOOP);
				glColor(col);
				for (unsigned int a = 0; a < 17; ++a) {
					float ang = a / 17.0f * float(M_PI) * 2.0f;
					glVertex(at + s*make_vector(cosf(ang),sinf(ang)));
				}
				glEnd();
			}
		}
	}


	//disable scissor:
	glPopAttrib();


	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-1.5f, -1.0f);
	glVertex2f( 1.5f, -1.0f);
	glVertex2f( 1.5f,  1.0f);
	glVertex2f(-1.5f,  1.0f);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Path::draw");

}

void PathModule::calculate_path() {
	path_c.clear();
	//Ok, need to start somewhere, end somewhere hmm...
	//Let's try something equation-y for starters:
	const unsigned int Observed = 2;
	const unsigned int Hidden = 6;
	//Hidden variables will be something like:
	//[px vx ax py vy ay]
	float Dt = 1.0f; // / 30.0f;
	//Store matricies in row-major order:
	vector< float > P(Hidden*Observed, 0.0f);
	P[0*Hidden + 0] = 1.0f;
	P[1*Hidden + 3] = 1.0f;
	/*//DEBUG: Dump 'P':
	std::cout << "P:" << endl;
	for (unsigned int row = 0; row < Observed; ++row) {
		for (unsigned int col = 0; col < Hidden; ++col) {
			std::cout << ' ' << P[row*Hidden+col];
		}
		std::cout << endl;
	}*/


	vector< float > HiddenWeights(Hidden, 1.f);
	HiddenWeights[2] = 0.1f;
	HiddenWeights[5] = 0.1f;

	vector< float > F(Hidden*Hidden, 0.0f);
	F[0*Hidden + 0] = 1.0f;
	F[0*Hidden + 1] = Dt;
	F[0*Hidden + 2] = 0.5f * Dt * Dt;
	F[1*Hidden + 1] = 1.0f;
	F[1*Hidden + 2] = Dt;
	F[2*Hidden + 2] = 1.0f;
	F[3*Hidden + 3] = 1.0f;
	F[3*Hidden + 4] = Dt;
	F[3*Hidden + 5] = 0.5f * Dt * Dt;
	F[4*Hidden + 4] = 1.0f;
	F[4*Hidden + 5] = Dt;
	F[5*Hidden + 5] = 1.0f;
	/*//DEBUG: Dump 'F':
	std::cout << "F:" << endl;
	for (unsigned int row = 0; row < Hidden; ++row) {
		for (unsigned int col = 0; col < Hidden; ++col) {
			std::cout << ' ' << F[row*Hidden+col];
		}
		std::cout << endl;
	}*/

	std::cout << "Calculating path of length " << Window*2 + cur_intermed << "." << endl; //DEBUG
	//What have we seen recently:
	vector< float > observed_before(Observed*Window, 0.0f);
	vector< float > observed_after(Observed*Window, 1.0f);
	if (!path_a.empty()) {
		for (int ind = 0; (unsigned)ind < Window; ++ind) {
			int pos = (int)cur_leave_a + ind + 1 - Window;
			Vector2f pt;
			if (pos < 0) pt = path_a[0];
			else if ((unsigned)pos >= path_a.size()) pt = path_a.back();
			else pt = path_a[pos];
			observed_before[ind*Observed+0] = pt.x;
			observed_before[ind*Observed+1] = pt.y;
		}
	}
	if (!path_b.empty()) {
		for (int ind = 0; (unsigned)ind < Window; ++ind) {
			int pos = (int)cur_join_b + ind;
			Vector2f pt;
			if (pos < 0) pt = path_b[0];
			else if ((unsigned)pos >= path_b.size()) pt = path_b.back();
			else pt = path_b[pos];
			observed_after[ind*Observed+0] = pt.x;
			observed_after[ind*Observed+1] = pt.y;
		}
	}
	//Now we've got to minimize a nice sum-of-squares:
	// (Px_i - z_i)^2 --> [0 .. Window-1, (skip), Window+intermed .. 2*Window-1]
	// (Fx_i - x_(i+1))^2 --> all points.

	vector< vector< float > > equations;
	for (unsigned int frame = 0; frame < cur_intermed + 2*Window; ++frame) {
		for (unsigned int var = 0; var < Hidden; ++var) {
			equations.push_back(vector< float >((cur_intermed + 2*Window)*Hidden + 1, 0.0f));
			vector< float > &eqn = equations.back();
			//Ok, take partials & add 'em in.
			//First eqn is (Fx_i - x_{i+1})^2
			// really:
			//   ((Fx_i)[n] - x_{i+1}[n])^2
			if (frame + 1 < cur_intermed + 2*Window) { //skip on last frame.
				for (unsigned int n = 0; n < Hidden; ++n) {
					float coef = F.at(n*Hidden + var) * HiddenWeights[n];
					if (coef == 0.0f) continue;
					for (unsigned int i = 0; i < Hidden; ++i) {
						eqn.at(frame*Hidden + i) += coef * F.at(n*Hidden + i);
					}
					eqn.at((frame+1)*Hidden + n) += coef * -1.0f;
				}
			}
			//Partial for the x_{i+1}[var]
			if (frame > 0) { //skip on first frame.
				float coef = -1.0f * HiddenWeights[var];
				for (unsigned int i = 0; i < Hidden; ++i) {
					eqn.at((frame-1)*Hidden + i) += coef * F.at(var*Hidden + i);
				}
				eqn.at(frame*Hidden + var) += coef * -1.0f;
			}
			/*
			//DEBUG: add simple smoothness term:
			//(x_i-x_{i+1})^2
			if (frame + 1 < cur_intermed + 2*Window) {
				eqn.at(frame*Hidden + var) += 1.0f;
				eqn.at((frame+1)*Hidden + var) += -1.0f;
			}
			if (frame > 0) {
				eqn.at((frame-1)*Hidden + var) += -1.0f;
				eqn.at(frame*Hidden + var) += 1.0f;
			}
			//end DEBUG.
			*/
			//Second eqn is (Px_i - z_i)^2:
			if (frame < Window) {
				for (unsigned int obs = 0; obs < Observed; ++obs) {
					float coef = P[obs*Hidden+var];
					if (coef == 0.0f) continue;
					for (unsigned int i = 0; i < Hidden; ++i) {
						eqn.at(frame*Hidden + i) += coef * P.at(obs*Hidden + i);
					}
					eqn.back() += coef * observed_before[frame*Observed + obs];
				}
			}
			if (frame >= cur_intermed + Window) {
				for (unsigned int obs = 0; obs < Observed; ++obs) {
					float coef = P[obs*Hidden+var];
					if (coef == 0.0f) continue;
					for (unsigned int i = 0; i < Hidden; ++i) {
						eqn.at(frame*Hidden + i) += coef * P.at(obs*Hidden + i);
					}
					eqn.back() += coef * observed_after.at((frame-(cur_intermed+Window))*Observed + obs);
				}
			}
		} //for (var)
	} //for (frame)
	assert(equations.size() + 1 == equations[0].size());

	/*//DEBUG: dump equations.
	std::cout << "Before:" << endl;
	for (unsigned int row = 0; row < equations.size(); ++row) {
		for (unsigned int col = 0; col < equations[row].size(); ++col) {
			if (col) std::cout << ' ';
			std::cout << equations[row][col];
		}
		std::cout << endl;
	}*/


	//Now to solve our large system of equations, eh?
	for (unsigned int row = 0; row < equations.size(); ++row) {
		unsigned int selected_row = (unsigned)-1;
		float max_val = 0.0f;
		//row with largest elt in column == 'row'
		for (unsigned int test_row = row; test_row < equations.size(); ++test_row) {
			float test_val = fabs(equations[test_row][row]);
			if (test_val > max_val) {
				selected_row = test_row;
				max_val = test_val;
			}
		}
		if (selected_row >= equations.size()) {
			cerr << "Problem in row-reduction. Drats." << endl;
			continue;
		}
		if (selected_row != row) {
			equations[row].swap(equations[selected_row]);
		}
		float inv_val = 1.0f / equations[row][row];
		for (unsigned int col = row; col < equations[0].size(); ++col) {
			equations[row][col] *= inv_val;
		}
		for (unsigned int target_row = 0; target_row < equations.size(); ++target_row) {
			if (target_row == row) continue;
			float amt = equations[target_row][row];
			if (amt == 0.0f) continue;
			equations[target_row][row] = 0.0f;
			assert(equations[target_row].size() == equations[row].size());
			for (unsigned int col = row+1; col < equations[target_row].size(); ++col) {
				equations[target_row][col] -= amt * equations[row][col];
			}
		}
	}

	/*//DEBUG: dump equations.
	std::cout << "After:" << endl;
	for (unsigned int row = 0; row < equations.size(); ++row) {
		for (unsigned int col = 0; col < equations[row].size(); ++col) {
			if (col) std::cout << ' ';
			std::cout << equations[row][col];
		}
		std::cout << endl;
	}*/

	float score_out = 0.0f;

	//Project estimated answer:
	vector< float > answer((cur_intermed + Window*2)*Observed, 0.0f);
	for (unsigned int frame = 0; frame < cur_intermed + Window*2; ++frame) {
		for (unsigned int obs = 0; obs < Observed; ++obs) {
			for (unsigned int var = 0; var < Hidden; ++var) {
				answer.at(frame * Observed + obs) += P.at(obs*Hidden + var) * equations.at(frame*Hidden + var).back();
			}
		}
		//Add score:
		if (frame > 0) {
			Vector2f xdd = make_vector(equations.at(frame*Hidden + 2).back(), equations.at(frame*Hidden + 5).back());
			Vector2f x = make_vector(equations.at(frame*Hidden + 0).back(), equations.at(frame*Hidden + 3).back());
			Vector2f prev_x = make_vector(equations.at((frame-1)*Hidden + 0).back(), equations.at((frame-1)*Hidden + 3).back());
			score_out += length(xdd) * length(prev_x - x);
		}
	}
	std::cout << score_out << std::endl;
	score() = score_out;
	//and copy to path:
	path_c.resize(cur_intermed + Window * 2);
	for (unsigned int frame = 0; frame < cur_intermed + Window * 2; ++frame) {
		path_c[frame].x = answer[frame * Observed + 0];
		path_c[frame].y = answer[frame * Observed + 1];
	}
}

void PathModule::update(float elapsed_time) {
	int new_leave_a = (int)(leave_a() + 0.5);
	if (new_leave_a < 0) new_leave_a = 0;
	if ((unsigned)new_leave_a >= path_a.size()) new_leave_a = path_a.size() - 1;

	int new_join_b = (int)(join_b() + 0.5);
	if (new_join_b < 0) new_join_b = 0;
	if ((unsigned)new_join_b >= path_b.size()) new_join_b = path_b.size() - 1;

	int new_intermed = (int)(intermed() + 0.5);
	if (new_intermed < 0) new_intermed = 0;

	if (cur_intermed != (unsigned)new_intermed
	 || cur_leave_a != (unsigned)new_leave_a
	 || cur_join_b != (unsigned)new_join_b) {
		cur_intermed = new_intermed;
		cur_leave_a = new_leave_a;
		cur_join_b = new_join_b;
		calculate_path();
	}
	if (animate) {
		anim_time += elapsed_time;
		if (anim_time * FPS > cur_leave_a + cur_intermed + (path_b.size() - cur_join_b)) {
			animate = false;
			anim_time = 0.0f;
		}
	}
}

bool PathModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		if (animate) {
			animate = false;
			anim_time = 0.0f;
		} else {
			animate = true;
			anim_time = 0.0f;
		}
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		show_c = !show_c;
	}
	return false;
}
