#include "DisplayModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

#include <sstream>

using std::ostringstream;

namespace {
	template< class DATA >
	Module *create_displaymodule(const std::string &params) {
		return new DisplayModule< DATA >(params);
	}
	class Fred {
	public:
		Fred() {
			register_module("display-f", create_displaymodule< float >);
			register_module("display-ui", create_displaymodule< unsigned int >);
			register_module("display-s", create_displaymodule< string >);
		}
	} fred;
}

template< class DATA >
Vector2f DisplayModule< DATA >::size() {
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	float len = gentium->get_length(prefix + "    ", 0.3f);
	return make_vector(len, 0.3f);
}

template< class DATA >
void DisplayModule< DATA >::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	glColor3f(1.0f, 1.0f, 1.0f);
	Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
	ostringstream msg;
	msg << prefix << data();
	gentium->draw(msg.str(), make_vector(-0.5f * size().x, -0.15f), 0.3f);

	glMatrixMode(GL_PROJECTION);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Display::draw");

}


template< class DATA >
void DisplayModule< DATA >::update(float elapsed_time) {
}

template< class DATA >
bool DisplayModule< DATA >::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	return false;
}
