#include "SS_OptCanvas.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/load_save_png.hpp>

#include <vector>
#include <set>
#include <fstream>
#include <sstream>

#include "SS_coefs.hpp"

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

using std::ifstream;
using std::ofstream;
using std::getline;
using std::istringstream;

namespace {
	Module *create_module(const std::string &params_in) {
		string base_name = "";
		string params = params_in;
		vector< string > files;
		params += ' ';
		while (params.size()) {
			if (isspace(params[0])) {
				params.erase(0,1);
				continue;
			}
			for (unsigned int c = 0; c < params.size(); ++c) {
				if (isspace(params[c])) {
					files.push_back(params.substr(0,c));
					params.erase(0,c);

					if (files.back().substr(0,1) == "#") {
						base_name = files.back().substr(1);
						files.pop_back();
					}
					break;
				}
			}
		}
		LOG_INFO("Adding " << files.size() << " files.");
		OptStacking *ss = new OptStacking(base_name);
		for (unsigned int f = 0; f < files.size(); ++f) {
			string arg = files[f];
			if (arg.size() > 4 && arg.substr(arg.size()-4,4) == ".png") {
				bool perm = false;
				if (arg.substr(0,1) == "*") {
					//use permeability map!
					perm = true;
					arg = arg.substr(1);
				}
				Layer l;
				Vector2ui size;
				if (!load_png(arg, size.x, size.y, l.data)) {
					LOG_WARNING("Not adding layer for '" << arg << "'");
				} else {
					if (ss->layers.empty()) {
						ss->img_size = size;
					}
					if (ss->img_size != size) {
						LOG_INFO("Layer '" << arg << "' is " << size << " != " << ss->img_size << ".");
					} else {
						LOG_INFO("Loaded '" << arg << "'");
						LOG_INFO("Layer is " << sizeof(Layer));
						ss->layers.push_back(l);
						if (perm) {
							ss->layers.back().perm_from_color(size);
						} else {
							ss->layers.back().perm_from_alpha(size);
						}
					}
				}
			} else {
				LOG_WARNING("Ignoring argument: '" << arg << "'");
			}
		}
		return ss;
	}
	class Fred {
	public:
		Fred() {
			register_module("ss_optimal", create_module, "layer0 layer1 ...");
		}
	} fred;
}

using std::set;
using std::vector;
using std::make_pair;

REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_EXT_blend_func_separate )


namespace {
void alloc_tex(Vector2ui size, GLuint *tex, GLenum type = GL_RGBA) {
	assert(tex);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	vector< uint32_t > blank(size.x * size.y, 0);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, type, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &blank[0]);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc_tex");
}

void alloc_fb(GLuint tex, GLuint *fb) {
	assert(fb);
	glGenFramebuffersEXT(1, fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
	Graphics::gl_errors("fb setup");
	{ //check:
		GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
		if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
			LOG_WARNING("WARNING: order FRAMEBUFFER not complete!");
		}
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	Graphics::gl_errors("alloc_fb");
}
}

OptStacking::OptStacking(std::string _base_name) : base_name(_base_name) {
	img_size = make_vector(0U, 0U);
	soln_size = make_vector(0U, 0U);

	cursor_fade = 0.0f;
	have_cursor = false;

	solve = NULL;

	soln_tex = 0;
	cons_tex = 0;
	cons_fb = 0;
	brush_tex = 0;

	soln_serial = 0;

	cur_mouse = make_vector(0.0f, 0.0f);

	stroke_acc = 0.0f;
	brush_diameter = 10.0f;

	brush_spin_acc = 0.0f;
	stipple_ofs = 0.0f;

	brush_rate = 1.0f;

	layer_colors.name = "colors";
	layer_colors.position = make_vector(0.1f,-0.5f);

	closest_wanted.name = "cons";
	closest_wanted.position = make_vector(-0.1f,-0.5f);

	out_ports.push_back(&layer_colors);
	in_ports.push_back(&closest_wanted);

	free_colors.push_back(0xff391fdf);
	free_colors.push_back(0xfff1330d);
	free_colors.push_back(0xff81d02e);
	free_colors.push_back(0xffe7a02b);
	free_colors.push_back(0xff601483);
	free_colors.push_back(0xff86bfff);
	free_colors.push_back(0xff748a84);


}

OptStacking::~OptStacking() {
}

bool OptStacking::handle_event(SDL_Event const &e, Vector2f ext_mouse) {
	if (e.type == SDL_MOUSEMOTION) {
		cur_mouse = (ext_mouse + size() * 0.5f) * img_size.y;
		if (!stroke.empty()) {
			stroke.push_back(make_vector(cur_mouse, 1.0f)); 
			return true;
		}
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELUP) {
		brush_diameter *= 1.1f;
		return true;
	}
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_WHEELDOWN) {
		brush_diameter *= 1 / 1.1f;
		if (brush_diameter < 1.0f) brush_diameter = 1.0f;
		return true;
	}

	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		stroke_acc = 0.0f;
		stroke.push_back(make_vector(cur_mouse, 1.0f)); 
		stroke.push_back(make_vector(cur_mouse, 1.0f)); 
		return true;
	}
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		if (!stroke.empty()) {
			render_stroke();
			stroke.clear();
			return true;
		}
	}

	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
		if (solve && solve->is_running()) {
			delete solve;
			solve = NULL;
		} else {
			start_solve(e.key.keysym.mod & KMOD_SHIFT);
		}
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
		if (cons_fb) {
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
		constraints.clear();
		start_solve(false);
		return true;
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
		string name = base_name;
		if (name == "") {
			name = "opt_dump";
		}
		LOG_INFO("Saving to '" << name << "*'");
		{
			vector< uint32_t > image(soln_size.x * soln_size.y);
			glBindTexture(GL_TEXTURE_2D, soln_tex);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
			glBindTexture(GL_TEXTURE_2D, 0);
			save_png(name + ".png", soln_size.x, soln_size.y, &image[0]);
		}
		{
			vector< uint32_t > image(img_size.x * img_size.y);
			glBindTexture(GL_TEXTURE_RECTANGLE, cons_tex);
			glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
			glBindTexture(GL_TEXTURE_RECTANGLE, 0);
			save_png(name + "_cons.png", img_size.x, img_size.y, &image[0]);
		}
		{
			ofstream cons_desc((name + "_cons.txt").c_str());
			for (unsigned int i = 0; i < constraints.size(); ++i) {
				cons_desc << constraints[i].col;
				for (unsigned int v = 0; v < constraints[i].valid_orders.size(); ++v) {
					cons_desc << ' ' << constraints[i].valid_orders[v];
				}
				cons_desc << '\n';
			}
		}
		return true;
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_c) {
		string name = base_name;
		if (name == "") {
			name = "opt_dump";
		}
		if (solve) {
			vector< uint32_t > soln;
			Vector2ui soln_size;
			vector< vector< uint32_t > > soln_coefs;
			solve->get_current_solution(soln, soln_size, &soln_coefs);
			if (soln_coefs.size() == count_stackings(layers.size())) {
				for (unsigned int oi = 0; oi < soln_coefs.size(); ++oi) {
					vector< uint32_t > order = to_stacking(oi, layers.size());
					std::ostringstream fn;
					fn << name << ".coef";
					for (vector< uint32_t >::iterator o = order.begin(); o != order.end(); ++o) {
						fn << "-" << *o;
					}
					fn << ".png";
					assert(soln_coefs[oi].size() == soln_size.x * soln_size.y);
					save_png(fn.str(), soln_size.x, soln_size.y, &(soln_coefs[oi][0]));
				}
			}
		}
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_l) {
		string name = base_name;
		if (name == "") {
			name = "opt_dump";
		}
		LOG_INFO("Loading from '" << name << "*'");
		constraints.clear();
		{
			ifstream cons_desc((name + "_cons.txt").c_str());
			string line;
			while (getline(cons_desc, line)) {
				istringstream line_str(line);
				Const cons;
				if (!(line_str >> cons.col)) {
					LOG_ERROR("Um, missing color?");
					continue;
				}
				unsigned int order;
				while (line_str >> order) {
					cons.valid_orders.push_back(order);
				}
				constraints.push_back(cons);
			}
		}

		Vector2ui loaded_size = make_vector(0U, 0U);
		vector< uint32_t > loaded;
		if (load_png(name + "_cons.png", loaded_size.x, loaded_size.y, loaded)) {
			if (loaded_size != img_size) {
				LOG_WARNING("Can't load; different size.");
			} else {
				if (cons_tex == 0) {
					alloc_tex(img_size, &cons_tex, GL_RGBA);
					alloc_fb(cons_tex, &cons_fb);
				}
				glBindTexture(GL_TEXTURE_RECTANGLE, cons_tex);
				glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, img_size.x, img_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(loaded[0]));
				glBindTexture(GL_TEXTURE_RECTANGLE, 0);
			}
		}
		return true;
	}

	return false;
}

//Taken from soft_paint, because, um, that's just how I roll.
void OptStacking::render_stroke() {
	if (stroke.size() < 2) return;
	//brush as series of splatted 'brush' pictures:
	vector< Vector2f > quads;
	vector< Vector2f > quads_tex;
	for (unsigned int i = 0; i + 1 < stroke.size(); ++i) {
		Vector3f a = stroke[i];
		Vector3f b = stroke[i+1];
		float len = length(a-b);
		if (len == 0.0f) continue;
		float at = 0.0f;
		while (1) {
			float brush_start = ((b.z - a.z) * (at / len) + a.z) * 0.5f * brush_diameter * brush_rate;
			float brush_slope = (b.z - a.z) / float(len) * 0.5f * brush_diameter * brush_rate;
			float limit = len;
			if (brush_start < 0.5f) {
				brush_start = 0.5f;
				brush_slope = 0.0f;
				//figure out limit:
				if (brush_slope > 0 && brush_slope * (len - at) + brush_start > 0.5f) {
					limit = ((0.5f - brush_start) / brush_slope) + at;
					assert(limit <= len + 0.001f);
				}
			}
			//when is stroke_acc + at == brush_start + brush_slope * at?
			//(stroke_acc - brush_start) == (brush_slope - 1.0) * at

			if ((brush_slope - 1.0f) != 0.0f) {
				float test = (stroke_acc - brush_start) / (brush_slope - 1.0f) + at;
				if (test < limit) {
					at = test;
					stroke_acc = 0.0f;
					float size = ((b.z - a.z) * (at / len) + a.z) * 0.5f * brush_diameter;
					Vector2f c = (b.xy - a.xy) * (at / len) + a.xy;
					quads.push_back(make_vector(-1.0f,-1.0f)*size+c);
					quads.push_back(make_vector(-1.0f, 1.0f)*size+c);
					quads.push_back(make_vector( 1.0f, 1.0f)*size+c);
					quads.push_back(make_vector( 1.0f,-1.0f)*size+c);
					quads_tex.push_back(make_vector(0.0f,0.0f));
					quads_tex.push_back(make_vector(0.0f,1.0f));
					quads_tex.push_back(make_vector(1.0f,1.0f));
					quads_tex.push_back(make_vector(1.0f,0.0f));
					continue;
				}
			}
			stroke_acc += (len - at);
			break;
		}

	}

	if (quads.empty()) return;

	
	//empty out stroke queue:
	stroke.erase(stroke.begin(), stroke.begin() + stroke.size()-1);

	if (closest_wanted().size() != count_stackings(layers.size())) {
		//Whoops, can't draw.
		return;
	}
	assert(closest_wanted().size() == count_stackings(layers.size()));

	//What "order" are we drawing with?
	unsigned int color = 0xffffffff;
	{
		vector< uint32_t > valid_orders;
		{
			set< uint32_t > valid;
			for (unsigned int i = 0; i < closest_wanted().size(); ++i) {
				valid.insert(closest_wanted()[i]);
			}
			valid_orders.insert(valid_orders.begin(), valid.begin(), valid.end());
		}
		if (valid_orders.size() == count_stackings(layers.size())) {
			color = 0x00000000;
		} else {
			unsigned int found = -1U;
			for (unsigned int c = 0; c < constraints.size(); ++c) {
				if (constraints[c].valid_orders.size() != valid_orders.size()) continue;
				bool match = true;
				for (unsigned int v = 0; v < valid_orders.size(); ++v) {
					if (valid_orders[v] != constraints[c].valid_orders[v]) {
						match = false;
						break;
					}
				}
				if (match) {
					found = c;
					break;
				}
			}
			if (found == -1U) {
				found = constraints.size();
				constraints.push_back(Const());
				constraints.back().valid_orders = valid_orders;
				if (free_colors.size()) {
					constraints.back().col = free_colors[0];
					free_colors.erase(free_colors.begin());
				} else {
					LOG_WARNING("Ran out of hand-picked constraint colors");
					constraints.back().col = rand() | 0xff000000;
				}
			}
			color = constraints[found].col;
		}
	}


	//bind quads to vertex pointer:
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &quads[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2f), &quads_tex[0]);

	//and render to cons:
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,img_size.x,img_size.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(-1.0f, -1.0f, 0.0f);
	glScalef(2.0f / img_size.x, 2.0f / img_size.y, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);

	Graphics::ProgramObjectRef cons_shader = NULL;
	if (cons_shader.ref == NULL) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ss/cons_frag.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		cons_shader = Graphics::get_program_object(frag);
		assert(cons_shader.ref);
		glUseProgramObjectARB(cons_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(cons_shader->handle, "brush"), 0);
		glUseProgramObjectARB(0);
	}


	//------------------------------------------------
	//render stroke to cons_tex.
	if (cons_tex == 0) {
		alloc_tex(img_size, &cons_tex, GL_RGBA);
		alloc_fb(cons_tex, &cons_fb);
	}

	glEnable(GL_TEXTURE_2D);
	if (brush_tex == 0) {
		glGenTextures(1, &brush_tex);
		const unsigned int BrushSize = 512;
		vector< uint32_t > brush(BrushSize * BrushSize);
		for (unsigned int y = 0; y < BrushSize; ++y) {
			for (unsigned int x = 0; x < BrushSize; ++x) {
				Vector2f p;
				p.x = (x + 0.5f) / float(BrushSize) - 0.5f;
				p.y = (y + 0.5f) / float(BrushSize) - 0.5f;
				if (length_squared(p) > 0.5f * 0.5f) {
					brush[y * BrushSize + x] = 0x00ffffff;
				} else {
					brush[y * BrushSize + x] = 0xffffffff;
				}
			}
		}
		glBindTexture(GL_TEXTURE_2D, brush_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		Vector4f black = make_vector(1.0f, 1.0f, 1.0f, 0.0f);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, black.c);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BrushSize, BrushSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &brush[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glBindTexture(GL_TEXTURE_2D, brush_tex);
	glEnable(GL_TEXTURE_2D);


	glDisable(GL_BLEND);
	glUseProgramObjectARB(cons_shader->handle);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, cons_fb);
	glColor4ub(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, color >> 24);
	glDrawArrays(GL_QUADS, 0, quads.size());
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glUseProgramObjectARB(0);
	glEnable(GL_BLEND);

	glDisable(GL_TEXTURE_2D);
	//------------------------------------------------

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPopAttrib();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	Graphics::gl_errors("stroke");
}


void OptStacking::update(float elapsed) {
	if (!solve && soln_tex == 0) {
		start_solve(false);
	}

	stipple_ofs = fmodf(stipple_ofs + elapsed, 1.0f);

	if (solve) {
		vector< uint32_t > soln;
		if (solve->get_new_solution(soln, soln_size, soln_serial)) {
			LOG_INFO("Got solution " << soln_serial);
			//Whoo, new solution!
			if (soln_size.x > 0 && soln_size.y > 0) {
				//even better, it actually has extent.
				if (soln_tex == 0) {
					glGenTextures(1, &soln_tex);
				}
				glBindTexture(GL_TEXTURE_2D, soln_tex);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, soln_size.x, soln_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &soln[0]);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	if (have_cursor) {
		cursor_fade += 10.0f * elapsed;
		if (cursor_fade > 1.0f) cursor_fade = 1.0f;
	} else {
		cursor_fade -= 4.0f * elapsed;
		if (cursor_fade < 0.0f) cursor_fade = 0.0f;
	}
	have_cursor = false;

	if (!stroke.empty()) {
		brush_spin_acc = fmodf(brush_spin_acc + elapsed, 1.0f);
	}

	if (layer_colors().size() != layers.size()) {
		layer_colors().clear();
		for (unsigned int l = 0; l < layers.size(); ++l) {
			Vector4f total = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
			for (Vector2ui p = make_vector(0U, 0U); p.y < img_size.y; ++p.y) {
				for (p.x = 0; p.x < img_size.x; ++p.x) {
					uint32_t col = layers[l].data[p.y * img_size.x + p.x];
					Vector4f color = make_vector< float >(
						col & 0xff,
						(col >> 8) & 0xff,
						(col >> 16) & 0xff,
						col >> 24
						) / 255.0f;
					total += make_vector(color.xyz * color.w, color.w);
				}
			}
			if (total.w != 0.0f) {
				layer_colors().push_back(total.xyz / total.w);
			} else {
				layer_colors().push_back(make_vector(0.0f, 0.0f, 0.0f));
			}
		}
	}

	//Strokes:
	if (!stroke.empty()) {
		//Paint some stacking weights:
		render_stroke();
	}
}

Vector2f OptStacking::size() {
	if (img_size.y == 0) return make_vector(0.5f, 0.5f);
	else return make_vector(float(img_size.x) / img_size.y, 1.0f);
}

float OptStacking::cursor_alpha() {
	have_cursor = true;
	return (1.0f - cursor_fade);
}

void OptStacking::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f * size().x, -0.5f * size().y, 0.0f);
	if (img_size.y != 0) {
		glScalef(1.0f / img_size.y, 1.0f / img_size.y, 1.0f);
	}


	{ //draw layer bounds
		if (solve && solve->is_running()) {
			glColor3f(0.9f, 0.1f, 0.1f);
			glLineWidth(3.0f);
		} else {
			glColor3f(0.5f, 0.1f, 0.1f);
		}
		glBegin(GL_LINE_LOOP);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(img_size.x, 0.0f);
		glVertex2f(img_size.x, img_size.y);
		glVertex2f(0, img_size.y);
		glEnd();
		glLineWidth(1.0f);
	}

	if (soln_tex) { //draw result
		glBindTexture(GL_TEXTURE_2D, soln_tex);

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f( 1.0f, 0.0f ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f( 1.0f, 1.0f ); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f( 0.0f, 1.0f ); glVertex2f(  0.0f, img_size.y);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	if (cons_tex) { //draw constraints
		vector< uint32_t > stipple(32, 0);
		vector< uint32_t > stipple2(32, 0);
		vector< uint32_t > stipple3(32, 0);
		unsigned int ofs = int(stipple_ofs * 32);
		for (unsigned int y = 0; y < 32; ++y) {
			for (unsigned int x = 0; x < 32; ++x) {
				if ((x + y + ofs) % 8 < 3) {
					stipple[y] |= (1 << x);
				}
				if ((x + y + ofs) % 8 == 3) {
					stipple2[y] |= (1 << x);
				}
				if ((x + y + ofs) % 8 == 7) {
					stipple3[y] |= (1 << x);
				}
			}
		}

		glEnable(GL_POLYGON_STIPPLE);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glPolygonStipple(reinterpret_cast< GLubyte * >(&stipple[0]));

		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, cursor_fade);
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f( img_size.x, 0.0f ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f( img_size.x, img_size.y ); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f( 0.0f, img_size.y ); glVertex2f(  0.0f, img_size.y);
		glEnd();


		glPolygonStipple(reinterpret_cast< GLubyte * >(&stipple2[0]));

		glBegin(GL_QUADS);
		glColor4f(0.0f, 0.0f, 0.0f, cursor_fade);
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f( img_size.x, 0.0f ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f( img_size.x, img_size.y ); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f( 0.0f, img_size.y ); glVertex2f(  0.0f, img_size.y);
		glEnd();


		glPolygonStipple(reinterpret_cast< GLubyte * >(&stipple3[0]));

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, cursor_fade);
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f( img_size.x, 0.0f ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f( img_size.x, img_size.y ); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f( 0.0f, img_size.y ); glVertex2f(  0.0f, img_size.y);
		glEnd();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glDisable(GL_POLYGON_STIPPLE);
	}


	{
		//brush
		Vector2f at = cur_mouse;
		unsigned int segs = (brush_diameter * 2.0f * M_PI) / 7.0f + 5.0f;
		if (segs % 2) segs += 1;
		glLineWidth(4.5f);
		glColor4f(0.0f, 0.0f, 0.0f, 0.2f * cursor_fade);
		glBegin(GL_LINE_LOOP);
		for (unsigned int a = 0; a < segs; ++a) {
			float ang = float(a + brush_spin_acc * 10.0f) / float(segs) * 2.0f * float(M_PI);
			Vector2f b = make_vector(cosf(ang), sinf(ang));
			glVertex(b * 0.5f * brush_diameter + at);
		}
		glEnd();
		glLineWidth(2.7f);
		glBegin(GL_LINES);
		for (unsigned int a = 0; a < segs; ++a) {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f * cursor_fade);
			float ang = float(a + brush_spin_acc * 10.0f) / float(segs) * 2.0f * float(M_PI);
			Vector2f b = make_vector(cosf(ang), sinf(ang));
			glVertex(b * 0.5f * brush_diameter + at);
		}
		glEnd();

		glLineWidth(1.0f);
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();


	Graphics::gl_errors("draw");
}

void OptStacking::start_solve(bool store_coefs) {
	vector< uint32_t > cons(img_size.x * img_size.y, 0);
	if (cons_tex) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, cons_tex);
		glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_UNSIGNED_BYTE, &cons[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		Graphics::gl_errors("read cons_tex");
	}
	Problem *problem = new Problem;
	problem->layers = layers;
	problem->size = img_size;
	{ //add constraints:
		bool did_warn = false;
		std::vector< uint32_t > ind(constraints.size(), -1U);
		for (unsigned int i = 0; i < cons.size(); ++i) {
			if ((cons[i] & 0xff000000) == 0) continue; //ignore transparent
			unsigned int found = -1U;
			for (unsigned int c = 0; c < constraints.size(); ++c) {
				if ((constraints[c].col & 0xffffff) == (cons[i] & 0xffffff)) {
					found = c;
					break;
				}
			}
			if (found != -1U) {
				if (ind[found] == -1U) {
					ind[found] = problem->constraints.size();
					problem->constraints.push_back(Constraint());
					problem->constraints.back().valid_orders = constraints[found].valid_orders;
					problem->constraints.back().data.resize(img_size.x * img_size.y, 0);
				}
				problem->constraints[ind[found]].data[i] = 0xffffffff;
			} else {
				if (!did_warn) {
					LOG_WARNING("Didn't find constraint for color.");
					did_warn = true;
				}
			}
		}
	}
	if (solve) {
		delete solve;
		solve = NULL;
	}
	LOG_INFO("Have " << problem->layers.size() << " layers and " << problem->constraints.size() << " constraints, both of size " << img_size << "."); //DEBUG
	soln_serial = 0;
	solve = start_problem(problem, store_coefs);
}
