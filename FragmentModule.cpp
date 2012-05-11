#include "FragmentModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <Library/Library.hpp>
#include <Character/Draw.hpp>

#include "shapes.hpp"

#include <sstream>

using std::cout;
using std::istringstream;

unsigned int CurrentFragment = (unsigned)-1;

namespace {
	Module *create_fragmentmodule(const std::string &params) {
		Game2d::init_gamestuff();
		istringstream p(params);
		unsigned int index = 0;
		if (!(p >> index)) {
			cerr << "No index parameter given." << endl;
		}
		if (index >= Game2d::fragments.size()) {
			cerr << "Index '" << index << "' is out of range." << endl;
			return NULL;
		}
		return new FragmentModule(index);
	}
	class Fred {
	public:
		Fred() {
			register_module("fragment", create_fragmentmodule);
		}
	} fred;
}

Vector2f FragmentModule::size() {
	return make_vector(1.5f, 3.5f);
}

void FragmentModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
	Game2d::Fragment const &frag = Game2d::fragments.at(index());
	Library::Motion const &m = Library::motion(frag.motion);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(0.0f, -0.5f * size.y + 0.5f, 0.0f);
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
	unsigned int frame = (unsigned int)((time_acc / Game2d::Delta) * (frag.end_frame - frag.start_frame) + frag.start_frame);
	if (frame >= m.frames()) frame = m.frames() - 1;
	m.get_local_pose(frame, current_pose);
	state.orientation = 0.5f * float(M_PI);
	//skin.calculate(current_pose, state);


	//floor
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	glColor3f(0.7f, 0.7f, 0.7f);
	glVertex3f(size.x * 0.5f, 0.0f, -0.5f);
	glVertex3f(size.x *-0.5f, 0.0f, -0.5f);
	glColor3f(0.4f, 0.4f, 0.4f);
	glVertex3f(size.x *-0.5f, 0.0f,  0.5f);
	glVertex3f(size.x * 0.5f, 0.0f,  0.5f);
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
	glVertex3f(size.x * 0.5f, 0.0f, -0.5f);
	glVertex3f(size.x *-0.5f, 0.0f, -0.5f);
	glColor3f(0.4f, 0.4f, 0.4f);
	glVertex3f(size.x *-0.5f,-0.25f, -0.5f);
	glVertex3f(size.x * 0.5f,-0.25f, -0.5f);
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	//skin.draw();
	Game2d::enable_skin_shader();
	Character::draw(current_pose, state, false, false);
	Game2d::disable_skin_shader();
	glDisable(GL_DEPTH_TEST);

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
	Graphics::gl_errors("Fragment::draw");

}


void FragmentModule::update(float elapsed_time) {
	if (in_index() < Game2d::fragments.size()) {
		index() = in_index();
		in_index() = (unsigned)-1;
	}
	if (!paused) {
		time_acc += 0.1f * elapsed_time;
		if (time_acc > Game2d::Delta) {
			time_acc = 0.0f;
			paused = true;
		}
	}
}

bool FragmentModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		paused = false;
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		CurrentFragment = (unsigned)-1;
		paused = true;
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT))) {
		paused = false;
		int amt = event.motion.yrel;
		amt = amt % (signed)Game2d::fragments.size();
		if (amt < 0) amt += Game2d::fragments.size();
		index() = (index() + amt) % Game2d::fragments.size();
		CurrentFragment = index();
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP) {
		index() = (index() + Game2d::fragments.size() - 1) % Game2d::fragments.size();
		paused = false;
		CurrentFragment = index();
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN) {
		index() = (index() + 1) % Game2d::fragments.size();
		paused = false;
		CurrentFragment = index();
		return true;
	}
	return false;
}
