#include "LL_Code.hpp"

#include "LL_Regions.hpp"

#include <cassert>

namespace {
	Module *create_module(const std::string &params_in) {
		return new Code();
	}
	class Fred {
	public:
		Fred() {
			register_module("ll_code", create_module, "");
		}
	} fred;
}

namespace {
void draw_nothing(const PortData *, const Connection &, void *) {
}
}

Code::Code() {
	in_ports.push_back(&state);
	state.name = "state";
	state.position = make_vector(0.0f, 0.0f);
	state.draw_fn_data = NULL;
	state.draw_fn = draw_nothing;
	state() = NULL;
	code = Graphics::get_texture("ll_siggraph/flip-up.png", false, false);
	assert(code.ref);
}
Code::~Code() {
}
Vector2f Code::size() {
	return make_vector(1.0f, code->h / float(code->w));
}

void Code::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	assert(code.ref);
	glBindTexture(GL_TEXTURE_2D, code->obj);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	Box2f draw;
	draw.min = draw.max = make_vector(0.0f, 0.0f);
	if (state()) {
		switch( state()->state ) {
			case INIT:
				draw.min = make_vector< float >(65, 7);
				draw.max = draw.min + make_vector< float >(283, 39);
				break;
			case CHECK:
				draw.min = make_vector< float >(114, 45);
				draw.max = draw.min + make_vector< float >(276, 42);
				break;
			case CHECK_RETURN:
				draw.min = make_vector< float >(391, 47);
				draw.max = draw.min + make_vector< float >(109, 38);
				break;
			case WHILE:
				draw.min = make_vector< float >(114, 87);
				draw.max = draw.min + make_vector< float >(285, 42);
				break;
			case WHILE_RETURN:
			//	draw.min = make_vector< float >( , );
			//	draw.max = draw.min + make_vector< float >( , );
				break;
			case SWAP:
			case SWAP_ACTION:
			case SWAP_AFTER:
				draw.min = make_vector< float >(163, 165);
				draw.max = draw.min + make_vector< float >(285, 42);
				break;
			case RECURSE:
			case RECURSE_AFTER:
				draw.min = make_vector< float >(157, 206);
				draw.max = draw.min + make_vector< float >(346, 81);
				break;
			default:
			break;
		}
	}

	if (draw.min != draw.max) {
		draw.min.x = (draw.min.x / float(code->w) - 0.5f) * size().x;
		draw.max.x = (draw.max.x / float(code->w) - 0.5f) * size().x;
		draw.min.y = (1.0f - draw.min.y / float(code->h) - 0.5f) * size().y;
		draw.max.y = (1.0f - draw.max.y / float(code->h) - 0.5f) * size().y;
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 0.2f, 0.5f);
		glVertex2f(draw.min.x, draw.min.y);
		glVertex2f(draw.min.x, draw.max.y);
		glVertex2f(draw.max.x, draw.max.y);
		glVertex2f(draw.max.x, draw.min.y);
		glEnd();
	}


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}
void Code::update(float elapsed_time) {
}

bool Code::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}

