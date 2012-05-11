#include "InputModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>

using std::ostringstream;

namespace {
	template< class DATA >
	Module *create_inputmodule(const std::string &params) {
		return new InputModule< DATA >(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("input-f", create_inputmodule< float >);
		}
	} fred;
}

template< class DATA >
Vector2f InputModule< DATA >::size() {
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length(prefix + "    ", 0.3f);
	return make_vector(len, 0.3f);
}

template< class DATA >
void InputModule< DATA >::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glColor3f(1.0f, 1.0f, 1.0f);
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	ostringstream msg;
	msg << prefix << data();
	gentium->draw(msg.str(), make_vector( 0.5f * size().x - gentium->get_length(msg.str(), 0.3f), -0.15f), 0.3f);

	glMatrixMode(GL_PROJECTION);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Input::draw");

}


template< class DATA >
void InputModule< DATA >::update(float elapsed_time) {
}

template< class DATA >
bool InputModule< DATA >::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_MOUSEMOTION && event.motion.state) {
		data() += event.motion.y * 0.01f;
		return true;
	}
	return false;
}
