#include "PhysicsAnimation.hpp"

#include <Graphics/load_save_png.hpp>
#include <Vector/VectorGL.hpp>

using std::vector;
using std::string;

namespace {
	Module *create_animation(const std::string &params) {
		return new PhysicsAnimationModule(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("physanim", create_animation, "filename.png another.png");
		}
	} fred;
}


PhysicsAnimationModule::PhysicsAnimationModule(std::string params) : t(0.0f), dt(0.0f), up(make_vector(0.0f, 1.0f)), dragging(false), dragging_ref(make_vector(0.0f, 0.0f)), draw_coms(false) {
	while (!params.empty()) {
		for (unsigned int i = 0; i + 4 <= params.size(); ++i) {
			if (params.substr(i,4) == ".png" && (i + 4 == params.size() || params[i+4] == ' ')) {
				add_frame(params.substr(0,i+4));
				params.erase(0,i+5); //get space, if exists.
				break;
			}
		}
	}
}

PhysicsAnimationModule::~PhysicsAnimationModule() {
}

void PhysicsAnimationModule::add_frame(std::string filename) {
	Vector2ui pix;
	vector< uint32_t > data;
	if (!load_png(filename, pix.x, pix.y, data)) {
		return;
	}
	assert(data.size() == pix.x * pix.y);
	Vector3f com_acc = make_vector(0.0f, 0.0f, 0.0f);
	for (unsigned int y = 0; y < pix.y; ++y) {
		for (unsigned int x = 0; x < pix.x; ++x) {
			float amt = (data[y * pix.x + x] >> 24) / 255.0f;
			Vector2f at = make_vector(float(x) + 0.5f - float(pix.x) * 0.5f, float(y) + 0.5f - float(pix.y) * 0.5f);
			at /= float(pix.y);
			com_acc += make_vector(amt * at, amt);
		}
	}
	if (com_acc.z > 0.0f) {
		com_acc.xy /= com_acc.z;
	}
	coms.push_back(com_acc.xy);
	textures.push_back(Graphics::get_texture(filename, false, false));
}

Vector2f PhysicsAnimationModule::size() {
	return make_vector(1.0f, 1.0f);
}

void PhysicsAnimationModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f sz = size();

	glBegin(GL_QUADS);
	glColor3f(0.1f, 0.1f, 0.12f);
	glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
	glColor3f(0.3f, 0.25f, 0.25f);
	glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
	glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
	glEnd();

	int frame = int(t + 0.5);
	if (frame >= int(textures.size())) {
		frame = int(textures.size()) - 1;
	}
	if (frame < 0) frame = 0;

	if (unsigned(frame) < textures.size() && textures[frame].ref) {
		glBindTexture(GL_TEXTURE_2D, textures[frame]->obj);
		Vector2f right = -perpendicular(up);
		right *= textures[frame]->w / float(textures[frame]->h);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex(-0.5f * right + -0.5f * up);
		glTexCoord2f(1.0f, 0.0f); glVertex( 0.5f * right + -0.5f * up);
		glTexCoord2f(1.0f, 1.0f); glVertex( 0.5f * right +  0.5f * up);
		glTexCoord2f(0.0f, 1.0f); glVertex(-0.5f * right +  0.5f * up);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (draw_coms) {
		float scale = 2.0f;
		Vector2f right = -perpendicular(up);
		glBegin(GL_LINE_STRIP);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (unsigned int i = 0; i < coms.size(); ++i) {
			glVertex(scale * coms[i].x * right + scale * coms[i].y * up);
		}
		glEnd();
		glPointSize(5.0f);
		glBegin(GL_POINTS);
		glColor3f(0.7f, 0.0f, 0.5f);
		for (unsigned int i = 0; i < coms.size(); ++i) {
			glVertex(scale * coms[i].x * right + scale * coms[i].y * up);
		}

		if (coms.size() >= 2) {
			int it = int(t);
			if (it < 0) it = 0;
			if (it + 1 >= int(coms.size())) it = coms.size() - 2;
			float ft = t - it;
			glColor3f(1.0f, 0.0f, 0.2f);
			Vector2f vert = ft * (coms[it+1] - coms[it]) + coms[it];
			glVertex(scale * vert.x * right + scale * vert.y * up);
		}


		glEnd();
		glPointSize(1.0f);
	}

	//frame:
	glBegin(GL_LINE_LOOP);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x, -0.5f * sz.y);
	glVertex2f( 0.5f * sz.x,  0.5f * sz.y);
	glVertex2f(-0.5f * sz.x,  0.5f * sz.y);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("PhysicsAnimationModule::draw");

}

void PhysicsAnimationModule::update(float elapsed_time) {
	if (coms.size() < 2) return;
	int it = int(t);
	if (it < 0) it = 0;
	if (it + 2 > int(coms.size())) {
		it = coms.size() - 2;
	}
	assert(unsigned(it) + 1 < coms.size());
	Vector2f prev_com = coms[it];
	Vector2f next_com = coms[it+1];
	Vector2f along = normalize(next_com - prev_com);
	
	Vector2f grav = make_vector(0.0f,-10.0f);
	grav = make_vector(grav * -perpendicular(up), grav * up);

	dt += (along * grav) * elapsed_time;
	t += dt * elapsed_time;

	if (t < 0) {
		t = 0;
		if (dt < 0.0f) {
			dt *= -0.1f;
		}
	}
	if (t > coms.size() - 1) {
		t = coms.size() - 1;
		if (dt > 0.0f) {
			dt *= -0.1f;
		}
	}
}

bool PhysicsAnimationModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			dragging = true;
			dragging_ref = local_mouse;
			return true;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP) {
		if (event.button.button == SDL_BUTTON_LEFT) {
			dragging = false;
			return true;
		}
	}
	if (event.type == SDL_MOUSEMOTION) {
		if (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			if (dragging) {
				Vector2f dir = normalize(dragging_ref);
				Vector2f to = normalize(local_mouse);
				up = normalize(up * (to * dir) + perpendicular(up) * (to * perpendicular(dir)));
				dragging_ref = local_mouse;
				return true;
			}
		} else {
			dragging = false;
		}
	}
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_SPACE) {
			draw_coms = !draw_coms;
			return true;
		}
	}
	return false;
}
