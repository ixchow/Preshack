#include "LevelModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <Library/Library.hpp>
#include <Character/Draw.hpp>

#include "shapes.hpp"

#include <sstream>

using std::cout;
using std::istringstream;

namespace {
	Module *create_levelmodule(const std::string &params) {
		Game2d::init_gamestuff();
		return new LevelModule();
	}
	class Fred {
	public:
		Fred() {
			register_module("level", create_levelmodule);
		}
	} fred;
}

Vector2f LevelModule::size() {
	return make_vector(float(Game2d::tile_heights.size()), 5.0f);
}

void LevelModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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

	glEnable(GL_SCISSOR_TEST);

	//Library::Level const &m = Library::motion(index);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f * Game2d::tile_heights.size(), -0.5f * size.y + 0.5f, 0.0f);
	float mat[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	glMultMatrixf(mat);

	//Character::Skin &skin = Game2d::get_skin();
	bool do_square = (frame().motion >= Library::motion_count());
	Character::State state;
	if (do_square) {
		glBegin(GL_QUADS);
		glColor3f(1.0f, 0.0f, 1.0f);
		glVertex(status().pos + make_vector(-Game2d::PlayerRadius, Game2d::PlayerHeight));
		glColor3f(0.5f, 0.1f, 0.5f);
		glVertex(status().pos + make_vector( Game2d::PlayerRadius, Game2d::PlayerHeight));
		glColor3f(0.3f, 0.2f, 0.3f);
		glVertex(status().pos + make_vector( Game2d::PlayerRadius, 0.0f));
		glColor3f(0.5f, 0.1f, 0.5f);
		glVertex(status().pos + make_vector(-Game2d::PlayerRadius, 0.0f));
		glEnd();
	} else {
		state.clear();
		state.position = make_vector(status().pos.x, status().pos.y, 0.0f);
		state.orientation = 0.5f * float(M_PI);
		if (status().flip) state.orientation += 1.0f * float(M_PI);
		//skin.calculate(current_pose, state);
	}

	//floor
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	for (unsigned int i = 0; i < Game2d::tile_heights.size(); ++i) {
		float height = Game2d::tile_heights[i];
		if (height < 0.0f) continue;
		glColor3f(0.7f, 0.7f, 0.7f);
		glVertex3f(i + 1, height, -0.5f);
		glVertex3f(i    , height, -0.5f);
		glColor3f(0.4f, 0.4f, 0.4f);
		glVertex3f(i    , height,  0.5f);
		glVertex3f(i + 1, height,  0.5f);
	}
	glEnd();

	if (!do_square) {

		float min = status().pos.x - Game2d::PlayerRadius;
		float max = status().pos.x + Game2d::PlayerRadius;
		if (max > 0.0f && min < (float)Game2d::tile_heights.size()) {
			unsigned int m = (unsigned int)min;
			if (m >= Game2d::tile_heights.size()) m = 0;
			unsigned int M = (unsigned int)max;
			if (M >= Game2d::tile_heights.size()) M = Game2d::tile_heights.size() - 1;
			glEnable(GL_STENCIL_TEST);
			for (unsigned int i = m; i <= M; ++i) {
				float val = Game2d::tile_heights.at(i);
				glStencilFunc(GL_ALWAYS, i + 1, 0xff);
				glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
				glColor3f(1.0f, 0.0f, 1.0f);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glBegin(GL_QUADS);
				glVertex3f(i + 1, val, -0.5f);
				glVertex3f(i    , val, -0.5f);
				glVertex3f(i    , val,  0.5f);
				glVertex3f(i + 1, val,  0.5f);
				glEnd();
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

				glStencilFunc(GL_EQUAL, i + 1, 0xff);
				glStencilOp(GL_KEEP, GL_ZERO, GL_ZERO);
				glPushMatrix();
				glTranslatef(0.0f, val, 0.0f);
				glScalef(1.0f, 0.0f, 1.0f);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
				Character::draw(current_pose, state, false, false);
				glPopMatrix();
			}
			glDisable(GL_STENCIL_TEST);
		}

	}
	glDepthMask(GL_TRUE);
	//floor side.
	glBegin(GL_QUADS);
	for (unsigned int i = 0; i < Game2d::tile_heights.size(); ++i) {
		float height = Game2d::tile_heights[i];
		if (height < 0.0f) continue;
		glColor3f(0.6f, 0.6f, 0.6f);
		glVertex3f(i + 1, height, -0.5f);
		glVertex3f(i    , height, -0.5f);
		glColor3f(0.4f, 0.4f, 0.4f);
		glVertex3f(i    , -0.25, -0.5f);
		glVertex3f(i + 1, -0.25, -0.5f);
	}
	glEnd();

	if (!do_square) {
		glColor3f(1.0f, 1.0f, 1.0f);
		//skin.draw();
		Game2d::enable_skin_shader();
		Character::draw(current_pose, state, false, false);
		Game2d::disable_skin_shader();
	}
	glDisable(GL_DEPTH_TEST);
	glPopAttrib(); //back to previous scissor test.

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

	Graphics::gl_errors("Level::draw");
}


void LevelModule::update(float elapsed_time) {
	if (frame().motion >= Library::motion_count()) {
		current_pose.clear();
		return;
	}
	Library::Motion const &m = Library::motion(frame().motion);
	unsigned int f = frame().frame;
	if (f >= m.frames()) f = m.frames() - 1;
	Character::Pose target_pose;
	m.get_local_pose(f, target_pose);
	if (m.get_control(f).jump) {
		target_pose.root_position.y -= m.get_distance_to_floor(f);
	}
	if (current_pose.bone_orientations.size() != target_pose.bone_orientations.size()) {
		current_pose = target_pose;
	} else {
		{ //handle root position:
			float fac = 0.3f;
			current_pose.root_position += (target_pose.root_position - current_pose.root_position) * fac;
		}
		{ //handle root rotation:
			if (target_pose.root_orientation.xyzw * current_pose.root_orientation.xyzw < 0.0f) {
				target_pose.root_orientation.xyzw *= -1.0f;
			}
			float fac = 0.3f;
			current_pose.root_orientation = normalize(lerp(current_pose.root_orientation, target_pose.root_orientation, fac));
		}
		for (unsigned int i = 0; i < target_pose.bone_orientations.size(); ++i) {
			if (target_pose.bone_orientations[i].xyzw * current_pose.bone_orientations[i].xyzw < 0.0f) {
				target_pose.bone_orientations[i].xyzw *= -1.0f;
			}
			float fac = 0.3f;
			fac -= 0.2f * (1.0f - (target_pose.bone_orientations[i].xyzw * current_pose.bone_orientations[i].xyzw));
			//if (fac < 0.05f) fac = 0.05f;
			current_pose.bone_orientations[i] = normalize(lerp(current_pose.bone_orientations[i], target_pose.bone_orientations[i], fac));
		}
	}
}

bool LevelModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
