#include "MotionModule.hpp"
#include "FragmentModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <Library/Library.hpp>
#include <Character/Draw.hpp>
#include <Character/Skin.hpp>
#include <Character/skin_utils.hpp>

#include "shapes.hpp"

#include <sstream>

using std::cout;
using std::istringstream;
using std::swap;

namespace {
	Module *create_motionmodule(const std::string &params) {
		Game2d::init_gamestuff();
		istringstream p(params);
		unsigned int index = 0;
		if (!(p >> index)) {
			cerr << "No index parameter given." << endl;
		}
		if (index >= Game2d::selected_motions.size()) {
			cerr << "Index '" << index << "' is out of range." << endl;
			return NULL;
		}
		return new MotionModule(Game2d::selected_motions[index]);
	}
	class Fred {
	public:
		Fred() {
			register_module("motion", create_motionmodule);
		}
	} fred;
}

Vector2f MotionModule::size() {
	return make_vector(max_pos - min_pos + 1.0f, 3.7f);
}

void MotionModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f size = this->size();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glBegin(GL_QUADS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex3f(-0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x, -0.5f * size.y, 1.0f);
	glVertex3f( 0.5f * size.x,  0.5f * size.y, 1.0f);
	glVertex3f(-0.5f * size.x,  0.5f * size.y, 1.0f);
	glEnd();
	glDepthFunc(GL_LEQUAL);

	//do motion frames or something?
	Library::Motion const &m = Library::motion(index);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f * (max_pos + min_pos), -0.5f * size.y + 0.5f, 0.0f);
	float mat[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	glMultMatrixf(mat);

	Character::Pose current_pose;
	Character::State state;
	//Character::Skin &skin = Game2d::get_skin();

	state.clear();
	unsigned int frame = (unsigned int)(time_acc / m.skeleton->timestep);
	if (paused && CurrentFragment < Game2d::fragments.size()) {
		Game2d::Fragment const &frag = Game2d::fragments[CurrentFragment];
		if (index == frag.motion && frag.start_frame < frame_positions.size() && frag.end_frame < frame_positions.size()) {
			frame = (frag.start_frame + frag.end_frame) / 2;
		}
	}
	if (frame >= m.frames()) frame = m.frames() - 1;
	m.get_local_pose(frame, current_pose);
	state.position = make_vector(frame_positions.at(frame), 0.0f, 0.0f);
	state.orientation = 0.5f * float(M_PI);
	//skin.calculate(current_pose, state);

	//floor
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex3f(max_pos + 0.5f, 0.0f, -0.5f);
	glVertex3f(min_pos - 0.5f, 0.0f, -0.5f);
	glColor3f(0.4f, 0.4f, 0.4f);
	glVertex3f(min_pos - 0.5f, 0.0f,  0.5f);
	glVertex3f(max_pos + 0.5f, 0.0f,  0.5f);
	glEnd();

	glPushMatrix();
	glScalef(1.0f, 0.0f, 1.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	//skin.draw(false);
	Character::draw(current_pose, state, false, false);
	glPopMatrix();

	glDepthMask(GL_TRUE);
	//floor side.
	glBegin(GL_QUADS);
	glColor3f(0.6f, 0.6f, 0.6f);
	glVertex3f(max_pos + 0.5f, 0.0f, -0.5f);
	glVertex3f(min_pos - 0.5f, 0.0f, -0.5f);
	glColor3f(0.4f, 0.4f, 0.4f);
	glVertex3f(min_pos - 0.5f,-0.25f, -0.5f);
	glVertex3f(max_pos + 0.5f,-0.25f, -0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	//skin.draw();
	Game2d::enable_skin_shader();
	Character::draw(current_pose, state, false, false);
	Game2d::disable_skin_shader();
	glDisable(GL_DEPTH_TEST);

	if (CurrentFragment < Game2d::fragments.size()) {
		Game2d::Fragment const &frag = Game2d::fragments[CurrentFragment];
		if (index == frag.motion && frag.start_frame < frame_positions.size() && frag.end_frame < frame_positions.size()) {
			float start = frame_positions.at(frag.start_frame);
			float end = frame_positions.at(frag.end_frame);
			if (start > end) swap(start, end);
			glBegin(GL_LINE_LOOP);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3f(start - 0.5f, 0.0f, 0.0f);
			glVertex3f(  end + 0.5f, 0.0f, 0.0f);
			glVertex3f(  end + 0.5f, 3.0f, 0.0f);
			glVertex3f(start - 0.5f, 3.0f, 0.0f);
			glEnd();
		}
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(-0.5f * size.x, -0.5f * size.y);
	glVertex2f( 0.5f * size.x, -0.5f * size.y);
	glVertex2f( 0.5f * size.x,  0.5f * size.y);
	glVertex2f(-0.5f * size.x,  0.5f * size.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Motion::draw");

}


void MotionModule::update(float elapsed_time) {
	Library::Motion const &m = Library::motion(index);
	if (!paused) time_acc += elapsed_time;
	if (time_acc > m.length()) {
		time_acc = m.length();
		paused = true;
	}
}

bool MotionModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		time_acc = 0.0f;
		paused = false;
	}
	return false;
}

void MotionModule::init() {
	Library::Motion const &m = Library::motion(index);
	min_pos = max_pos = 0.0f;
	paused = true;
	float pos = 0.0f;
	frame_positions.clear();
	for (unsigned int f = 0; f < m.frames(); ++f) {
		frame_positions.push_back(pos);
		pos += m.get_control(f).desired_velocity.z * m.skeleton->timestep;
		if (pos < min_pos) min_pos = pos;
		if (pos > max_pos) max_pos = pos;
	}
}
