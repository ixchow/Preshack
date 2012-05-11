#include "LL_Dialog.hpp"

#include "LL_LayeringModule.hpp"

#include <Graphics/GLSLShader.hpp>

#include <cassert>

REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);

namespace {
	Module *create_module(const std::string &params_in) {
		return new LayersDialog();
	}
	class Fred {
	public:
		Fred() {
			register_module("ll_dialog", create_module, "");
		}
	} fred;
}

void draw_arrow(const PortData *, const Connection &, void *data) {
	LayersDialog *ld = (LayersDialog *)data;
	assert(ld);
	if (!ld->layering()) return;
	Vector2f arrow_c1 = ld->position + ld->scale * make_vector(-0.5f * ld->size().x, 0.5f * ld->size().y);
	Vector2f arrow_c2 = ld->position + ld->scale * make_vector(-0.5f * ld->size().x,-0.5f * ld->size().y);
	ld->layering()->draw_arrow(arrow_c1, arrow_c2);
}

LayersDialog::LayersDialog() {
	in_ports.push_back(&layering);
	layering.name = "layers";
	layering.position = make_vector(0.0f, 0.0f);
	layering.draw_fn_data = this;
	layering.draw_fn = draw_arrow;
	layering() = NULL;
	mouse_pos = make_vector(10.0f, 0.0f); //move out of the way of the bins.
}
LayersDialog::~LayersDialog() {
}
Vector2f LayersDialog::size() {
	if (!layering() || layering()->bins.empty()) {
		return make_vector(0.3f, 1.0f);
	} else {
		return make_vector(1.0f, layering()->bins.size() * BinHeight / BinWidth);
	}
}

namespace {
Graphics::ProgramObjectRef subpixel_shader = NULL;
}

void LayersDialog::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	if (!subpixel_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		subpixel_shader = Graphics::get_program_object(frag);
		assert(subpixel_shader.ref);
		glUseProgramObjectARB(subpixel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(subpixel_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}


	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor3f(0.4f, 0.2f, 0.2f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	if (!layering()) {
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		return;
	}

	vector< DialogBin > const &bins = layering()->bins;
	const float dialog_tween = layering()->dialog_tween;
	const unsigned int width = layering()->width;
	const unsigned int height = layering()->height;
	vector< GLuint > const &textures = layering()->textures;

	for (unsigned int b = 0; b < bins.size(); ++b) {
		float alpha = bins[b].active?1.0f:0.3f;
		Box2f box = bins[b].get_box(dialog_tween);
		glBegin(GL_QUADS);
		glColor4f(0.5f, 0.5f, 0.6f, alpha * 0.8f);
		glVertex2f(box.min.x, box.min.y);
		glVertex2f(box.max.x, box.min.y);
		glColor4f(0.9f, 0.85f, 0.83f, alpha * 0.8f);
		glVertex2f(box.max.x, box.max.y);
		glVertex2f(box.min.x, box.max.y);
		glEnd();
		Box2f tex = bins[b].get_tex_box(dialog_tween);
		Vector3f show = bins[b].get_show(dialog_tween, make_vector(width * 0.5f, height * 0.5f, width * 0.5f));
		show += bins[b].zoom_out * (make_vector(0.5f * width, 0.5f * height, 0.5f * width) - show);
		if (bins[b].layer < textures.size()) {
			float px_size = 2.0f / Graphics::screen_y * viewport.size().x / screen_viewport.size().x * show.z / (tex.max.y - tex.min.y);

			glUseProgramObjectARB(subpixel_shader->handle);
			glUniform1fARB(glGetUniformLocationARB(subpixel_shader->handle, "px_size"), px_size);

			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures[bins[b].layer]);
		}
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glBegin(GL_QUADS);
		glTexCoord2f(show.x-show.z,show.y-show.z);
		glVertex2f(tex.min.x, tex.min.y);
		glTexCoord2f(show.x+show.z,show.y-show.z);
		glVertex2f(tex.max.x, tex.min.y);
		glTexCoord2f(show.x+show.z,show.y+show.z);
		glVertex2f(tex.max.x, tex.max.y);
		glTexCoord2f(show.x-show.z,show.y+show.z);
		glVertex2f(tex.min.x, tex.max.y);
		glEnd();
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glUseProgramObjectARB(0);

		//arrows:
		float s = box.size().y * (1.0f / 3.0f);
		glEnable(GL_POLYGON_SMOOTH);
		glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f * alpha * bins[b].arrows);
		glVertex2f(box.max.x - s * 1.5f, box.max.y - 0.25 * s);
		glVertex2f(box.max.x - s * 0.75f, box.max.y - 1.25 * s);
		glVertex2f(box.max.x - s * 2.25f, box.max.y - 1.25 * s);

		glVertex2f(box.max.x - s * 1.5f, box.min.y + 0.25 * s);
		glVertex2f(box.max.x - s * 0.75f, box.min.y + 1.25 * s);
		glVertex2f(box.max.x - s * 2.25f, box.min.y + 1.25 * s);
		glEnd();
		glDisable(GL_POLYGON_SMOOTH);
		glBegin(GL_LINES);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f * alpha * bins[b].arrows);
		glVertex2f(tex.max.x + box.size().x * 0.05f, box.center().y);
		glVertex2f(box.max.x - box.size().x * 0.05f, box.center().y);
		glEnd();
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}
void LayersDialog::update(float elapsed_time) {
	if (layering() == NULL) return;


	vector< DialogBin > &bins = layering()->bins;
	float &dialog_tween = layering()->dialog_tween;
	dialog_tween += elapsed_time;
	if (dialog_tween > 1.0f) dialog_tween = 1.0f;
	for (unsigned int b = 0; b < bins.size(); ++b) {
		Box2f box = bins[b].get_box(dialog_tween);
		if (box.contains(mouse_pos)) {
			bins[b].arrows += elapsed_time * 4.0f;
			if (bins[b].arrows > 1.0f) bins[b].arrows = 1.0f;
		} else {
			bins[b].arrows -= elapsed_time * 1.5f;
			if (bins[b].arrows < 0.0f) bins[b].arrows = 0.0f;
		}
		Box2f tex = bins[b].get_tex_box(dialog_tween);
		if (tex.contains(mouse_pos)) {
			bins[b].zoom_out += elapsed_time * 3.0f;
			if (bins[b].zoom_out > 1.0f) bins[b].zoom_out = 1.0f;
		} else {
			bins[b].zoom_out -= elapsed_time * 3.0f;
			if (bins[b].zoom_out < 0.0f) bins[b].zoom_out = 0.0f;
		}
	}
}

bool LayersDialog::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	mouse_pos = local_mouse;
	if (layering() == NULL) return false;

	if (event.type == SDL_MOUSEBUTTONDOWN) {
		vector< DialogBin > const &bins = layering()->bins;
		ListGraph &stacking = layering()->stacking;
		const float dialog_tween = layering()->dialog_tween;
		const vector< vector< uint32_t > > &layers = layering()->layers;
		const Vector2ui dialog_point = layering()->dialog_point;
		const vector< uint32_t > &tags = layering()->tags;
		const unsigned int width = layering()->width;
		const unsigned int height = layering()->height;
		for (unsigned int b = 0; b < bins.size(); ++b) {
			Box2f box = bins[b].get_box(dialog_tween);
			if (box.contains(local_mouse)) {
				if (bins[b].layer >= layers.size()) continue;
				bool up = local_mouse.y > box.center().y;
				if (dialog_point.x < width && dialog_point.y < height && tags.size() == width * height) {
					unsigned int t = tags[dialog_point.y * width + dialog_point.x];
					assert(t < stacking.lists.size());
					for (unsigned int i = 0; i < stacking.lists[t].size(); ++i) {
						if (stacking.lists[t][i] == bins[b].layer) {
							if (up && i + 1 < stacking.lists[t].size()) {
								stacking.flip_rel(t, bins[b].layer, stacking.lists[t][i+1], true);
								layering()->update_image();
							}
							if (!up && i - 1 < stacking.lists[t].size()) {
								stacking.flip_rel(t, bins[b].layer, stacking.lists[t][i-1], false);
								layering()->update_image();
							}
							break;
						}
					}
				}
				return true;
			}
		}
	}
	return false;
}

