#include "SS_Canvas.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Texture.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/load_save_png.hpp>

#include <vector>

#include "SS_coefs.hpp"

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

namespace {
	Module *create_module(const std::string &params_in) {
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
					break;
				}
			}
		}
		LOG_INFO("Adding " << files.size() << " files.");
		SoftStacking *ss = new SoftStacking();
		for (unsigned int f = 0; f < files.size(); ++f) {
			string arg = files[f];
			if (arg.size() > 4 && arg.substr(arg.size()-4,4) == ".png") {
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
						ss->layers.push_back(l);
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
			register_module("ss_canvas", create_module, "layer0 layer1 ...");
		}
	} fred;
}


using std::vector;
using std::make_pair;

REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_EXT_blend_func_separate )



void alloc_tex(Vector2ui size, GLuint *tex, GLenum type = GL_RGBA) {
	assert(tex);
	vector< uint32_t > blank(size.x * size.y, 0);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, *tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, type, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &blank[0]);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc_tex");
}

//assumes buffer already bound.
void check_fb(const char *name) {
	GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
		LOG_WARNING("WARNING: " << name << " FRAMEBUFFER not complete!");
		switch (ret) {
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				LOG_WARNING("   Incomplete Attachment.");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				LOG_WARNING("   Missing Attachment.");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				LOG_WARNING("   Incomplete Dimensions.");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				LOG_WARNING("   Incomplete Formats.");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				LOG_WARNING("   Incomplete Draw Buffer.");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				LOG_WARNING("   Incomplete Read Buffer.");
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				LOG_WARNING("   Unsupported.");
				break;
		}
	}
}

void alloc_fb(GLuint tex, GLuint *fb, const char *name = "unknown") {
	assert(fb);
	glGenFramebuffersEXT(1, fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);
	Graphics::gl_errors("fb setup");
	check_fb(name);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	Graphics::gl_errors("alloc_fb");
}

Layer::Layer() : tex(0) {
}

void Layer::upload(Vector2ui size) {
	assert(tex == 0);
	assert(data.size() == size.x * size.y);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

Coef::Coef() : tex(0) {
}

void Coef::init(Vector2ui size, float val) {
	assert(tex == 0);
	vector< Vector4f > data(size.x * size.y, make_vector(val, val, val, val));
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef MACOSX
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_FLOAT, &(data[0]));
#else //!MACOSX
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_ALPHA, size.x, size.y, 0, GL_RGBA, GL_FLOAT, &(data[0]));
#endif //MACOSX
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc coef");
}


namespace {
	void draw_nothing(const PortData *, const Connection &, void *) {
	}
}

SoftStacking::SoftStacking() {
	img_size = make_vector(0U, 0U);
	dirty_region = make_box(make_vector(0U, 0U), make_vector(-1U, -1U));

	cursor_fade = 0.0f;
	have_cursor = false;

	image_tex = 0;
	image_fb = 0;
	stroke_tex = 0;
	stroke_fb = 0;
	order_tex = 0;
	order_fb = 0;
	temp_fb = 0;

	cur_mouse = make_vector(0.0f, 0.0f);

	stroke_acc = 0.0f;
	brush_diameter = 10.0f;

	brush_spin_acc = 0.0f;

	brush_rate = 1.0f;
	brush = 0;
	brushes.push_back(Graphics::get_texture("ss/soft.png", false, false));
	brushes.push_back(Graphics::get_texture("ss/hard.png", false, false));

	bg_tex = Graphics::get_texture("ss/bg.png", false, false);
	assert(bg_tex.ref);
	glBindTexture(GL_TEXTURE_2D, bg_tex->obj);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	assert(bg_tex.ref);

	alpha.name = "alpha";
	alpha.position = make_vector(0.0f, 0.5f);
	alpha() = 1.0;
	alpha.draw_fn = draw_nothing;

	layer_colors.name = "colors";
	layer_colors.position = make_vector(0.1f,-0.5f);

	closest_wanted.name = "cons";
	closest_wanted.position = make_vector(-0.1f,-0.5f);

	out_ports.push_back(&layer_colors);
	in_ports.push_back(&alpha);
	in_ports.push_back(&closest_wanted);
}

SoftStacking::~SoftStacking() {
}

bool SoftStacking::handle_event(SDL_Event const &e, Vector2f ext_mouse) {
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

	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_BACKSPACE) {
		coefs.clear();
		dirty_region.min = make_vector(0U, 0U);
		dirty_region.max = img_size;
		return true;
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_LEFT) {
		brush = brush - 1;
		if (brush >= brushes.size()) {
			brush = brushes.size() - 1;
		}
		return true;
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RIGHT) {
		brush = brush + 1;
		if (brush >= brushes.size()) {
			brush = 0;
		}
		return true;
	}
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
		vector< uint32_t > image(img_size.x * img_size.y);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, image_tex);
		glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		save_png("paint_dump.png", img_size.x, img_size.y, &image[0]);
		return true;
	}
	return false;
}

//Taken from soft_paint, because, um, that's just how I roll.
void SoftStacking::render_stroke() {
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

	//TODO: intelligently set dirty region so we don't re-compose entire image.
	dirty_region.min = make_vector(0U, 0U);
	dirty_region.max = img_size;

	
	//empty out stroke queue:
	stroke.erase(stroke.begin(), stroke.begin() + stroke.size()-1);

	if (closest_wanted().size() != count_stackings(layers.size())) {
		//Whoops, can't draw.
		return;
	}
	assert(closest_wanted().size() == count_stackings(layers.size()));
	assert(closest_wanted().size() == coefs.size());

	//bind quads to vertex pointer:
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(Vector2f), &quads[0]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vector2f), &quads_tex[0]);

	//and render to coefs:
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,img_size.x,img_size.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(-1.0f, -1.0f, 0.0f);
	glScalef(2.0f / img_size.x, 2.0f / img_size.y, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);


	vector< vector< uint32_t > > sources(coefs.size());
	for (unsigned int i = 0; i < closest_wanted().size(); ++i) {
		if (closest_wanted()[i] == i) {
			continue;
		}
		assert(closest_wanted()[i] < sources.size());
		sources[closest_wanted()[i]].push_back(i);
	}

	if (temp_fb == 0) {
		glGenFramebuffersEXT(1, &temp_fb);
	}

/* Probably not the right idea.
	//------------------------------------------------
	//accumulate the coefs we want to accum_tex so we
	//can normalize later.
	static GLuint accum_tex = 0;
	static GLuint accum_fb = 0;
	if (accum_tex == 0) {
		alloc_tex(size, &accum_tex, GL_RGB16F_ARB);
		alloc_fb(accum_tex, &accum_fb);
	}

	glBlendFuncSeparateEXT(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, accum_fb);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	for (unsigned int c = 0; c < wanted_orders.size(); ++c) {
		if (!wanted_orders[c]) continue;
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coefs[c].tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f(size.x, 0.0f  ); glVertex2f(size.x,   0.0f);
		glTexCoord2f(size.x, size.y); glVertex2f(size.x, size.y);
		glTexCoord2f(  0.0f, size.y); glVertex2f(  0.0f, size.y);
		glEnd();
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
*/

	//------------------------------------------------
	//render stroke to stroke_tex.
	if (stroke_tex == 0) {
#ifdef MACOSX
		alloc_tex(img_size, &stroke_tex, GL_RGBA);
#else //!MACOSX
		alloc_tex(img_size, &stroke_tex, GL_ALPHA);
#endif //MACOSX
		alloc_fb(stroke_tex, &stroke_fb, "stroke");
	}

	glEnable(GL_TEXTURE_2D);
	assert(brush < brushes.size());
	glBindTexture(GL_TEXTURE_2D, brushes[brush]->obj);
	glEnable(GL_TEXTURE_2D);


	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, stroke_fb);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//DEBUG: first was GL_ONE.
	glBlendFuncSeparateEXT(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, alpha());
	glDrawArrays(GL_QUADS, 0, quads.size());
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glDisable(GL_TEXTURE_2D);
	//------------------------------------------------
	

	Graphics::ProgramObjectRef stroke_shader = NULL;
	if (stroke_shader.ref == NULL) {
		vector< string > ins;
		#ifdef MACOSX
		ins.push_back("#define CHAN r");
		#else //!MACOSX
		ins.push_back("#define CHAN a");
		#endif //OSX
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object(ins, "ss/stroke_frag.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		stroke_shader = Graphics::get_program_object(frag);
		assert(stroke_shader.ref);
		glUseProgramObjectARB(stroke_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(stroke_shader->handle, "stroke"), 0);
		glUniform1iARB(glGetUniformLocationARB(stroke_shader->handle, "from"), 1);
		glUseProgramObjectARB(0);
	}
	//-----------------------------------------------

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, stroke_tex);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, temp_fb);
	
	//for everywhere we want stroke, add some energy:
	for (unsigned int c = 0; c < coefs.size(); ++c) {
		if (sources[c].empty()) continue;
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, coefs[c].tex, 0);
		Graphics::gl_errors("fb bind");
		check_fb("coef");
		for (unsigned int s = 0; s < sources[c].size(); ++s) {
			assert(sources[c][s] != c);
			//DEBUG: second was ONE_MINUS_SRC_ALPHA.
			glBlendFuncSeparateEXT(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
			glUseProgramObjectARB(stroke_shader->handle);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coefs[sources[c][s]].tex);
			glActiveTexture(GL_TEXTURE0);
			glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(  0.0f,   0.0f);
			glTexCoord2f(img_size.x, 0.0f  ); glVertex2f(img_size.x,   0.0f);
			glTexCoord2f(img_size.x, img_size.y); glVertex2f(img_size.x, img_size.y);
			glTexCoord2f(  0.0f, img_size.y); glVertex2f(  0.0f, img_size.y);
			glEnd();
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glActiveTexture(GL_TEXTURE0);
			glUseProgramObjectARB(0);
		}
	}
	//For everywhere we took stroke from, remove some energy:
	for (unsigned int c = 0; c < coefs.size(); ++c) {
		if (!sources[c].empty()) continue;
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, coefs[c].tex, 0);
		Graphics::gl_errors("fb bind");
		{ //check:
			GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
			if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
				LOG_WARNING("WARNING: coef FRAMEBUFFER not complete!");
			}
		}
		//just remove some amount for the brush:
		glBlendFuncSeparateEXT(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f(img_size.x, 0.0f  ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f(img_size.x, img_size.y); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f(  0.0f, img_size.y); glVertex2f(  0.0f, img_size.y);
		glEnd();
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPopAttrib();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	Graphics::gl_errors("stroke");
}


void SoftStacking::update(float elapsed) {

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

	//If we don't have storage for coefs... we should do that:
	while (coefs.size() < count_stackings(layers.size())) {
		coefs.push_back(Coef());
		if (coefs.size() == 1) {
			coefs.back().init(img_size, 1.0f);
		} else {
			coefs.back().init(img_size, 0.0f);
		}
	}

	//Strokes:
	if (!stroke.empty()) {
		//Paint some stacking weights:
		render_stroke();
	}

	if (dirty_region.min.x < dirty_region.max.x && dirty_region.min.y < dirty_region.max.y) {
		//Sanity bounds dirty region:
		if (dirty_region.max.x > img_size.x) {
			dirty_region.max.x = img_size.x;
		}
		if (dirty_region.max.y > img_size.y) {
			dirty_region.max.y = img_size.y;
		}
		//Make sure our layer textures are uploaded:
		for (unsigned int l = 0; l < layers.size(); ++l) {
			if (!layers[l].tex) layers[l].upload(img_size);
		}
		size_t stackings = count_stackings(layers.size());

		if (order_tex == 0) {
			alloc_tex(img_size, &order_tex, GL_RGBA);
			alloc_fb(order_tex, &order_fb, "order");
		}

		if (image_tex == 0) {
			alloc_tex(img_size, &image_tex, GL_RGBA16F);
			alloc_fb(image_tex, &image_fb, "image");
		}

		Graphics::ProgramObjectRef accumulate_shader = NULL;
		if (accumulate_shader.ref == NULL) {
			vector< string > ins;
			#ifdef MACOSX
			ins.push_back("#define CHAN r");
			#else //!MACOSX
			ins.push_back("#define CHAN a");
			#endif //OSX
			Graphics::ShaderObjectRef frag = Graphics::get_shader_object(ins, "ss/accumulate_frag.glsl", GL_FRAGMENT_SHADER_ARB);
			assert(frag.ref);
			accumulate_shader = Graphics::get_program_object(frag);
			assert(accumulate_shader.ref);

			glUseProgramObjectARB(accumulate_shader->handle);
			glUniform1iARB(glGetUniformLocationARB(accumulate_shader->handle, "image"), 0);
			glUniform1iARB(glGetUniformLocationARB(accumulate_shader->handle, "coef"), 1);
			glUseProgramObjectARB(0);
		}



		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,img_size.x,img_size.y);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, image_fb);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (unsigned int s = 0; s < stackings; ++s) {
			assert(s < coefs.size());
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, order_fb);
			glDisable(GL_BLEND);
			glBindTexture(GL_TEXTURE_2D, bg_tex->obj);
			glEnable(GL_TEXTURE_2D);
			glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 0.0f); //Note: I think this should be zero alpha...
			glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(-1.0f,-1.0f);
			glTexCoord2f(img_size.x / bg_tex->w, 0.0f  ); glVertex2f( 1.0f,-1.0f);
			glTexCoord2f(img_size.x / bg_tex->w, img_size.y / bg_tex->h); glVertex2f( 1.0f, 1.0f);
			glTexCoord2f(  0.0f, img_size.y / bg_tex->h); glVertex2f(-1.0f, 1.0f);
			glEnd();
			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);


			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			vector< uint32_t > order = to_stacking(s, layers.size());
			for (vector< uint32_t >::iterator o = order.begin(); o != order.end(); ++o) {
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, layers[*o].tex);
				glEnable(GL_TEXTURE_RECTANGLE_ARB);
				glBegin(GL_QUADS);
				glColor3f(1.0f, 1.0f, 1.0f);
				glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(-1.0f,-1.0f);
				glTexCoord2f(img_size.x, 0.0f  ); glVertex2f( 1.0f,-1.0f);
				glTexCoord2f(img_size.x, img_size.y); glVertex2f( 1.0f, 1.0f);
				glTexCoord2f(  0.0f, img_size.y); glVertex2f(-1.0f, 1.0f);
				glEnd();
				glDisable(GL_TEXTURE_RECTANGLE_ARB);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			Graphics::gl_errors("Composite");


			//Accumulate:
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, image_fb);
			glUseProgramObjectARB(accumulate_shader->handle);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coefs[s].tex);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, order_tex);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			glBegin(GL_QUADS);
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(-1.0f,-1.0f);
			glTexCoord2f(img_size.x, 0.0f  ); glVertex2f( 1.0f,-1.0f);
			glTexCoord2f(img_size.x, img_size.y); glVertex2f( 1.0f, 1.0f);
			glTexCoord2f(  0.0f, img_size.y); glVertex2f(-1.0f, 1.0f);
			glEnd();
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glActiveTexture(GL_TEXTURE0);
			glDisable(GL_TEXTURE_RECTANGLE_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			glUseProgramObjectARB(0);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			Graphics::gl_errors("Accumulate");
		}
		glPopAttrib();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		dirty_region = make_box(make_vector(-1U, -1U), make_vector(0U,0U));

		/*
		//DEBUG: see if we observe alpha channel issues:
		vector< Vector4f > image(img_size.x * img_size.y);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, image_tex);
		glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_FLOAT, &image[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		Vector4f m = image[0];
		Vector4f M = image[0];
		for (unsigned int i = 0; i < image.size(); ++i) {
			 m = min(m, image[i]);
			 M = max(M, image[i]);
		}
		LOG_INFO("image range " << m << " to " << M);
		*/
	}
}

Vector2f SoftStacking::size() {
	if (img_size.y == 0) return make_vector(0.5f, 0.5f);
	else return make_vector(float(img_size.x) / img_size.y, 1.0f);
}

float SoftStacking::cursor_alpha() {
	have_cursor = true;
	return (1.0f - cursor_fade);
}

void SoftStacking::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

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
		glBegin(GL_LINE_LOOP);
		glColor3f(0.5f, 0.1f, 0.1f);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(img_size.x, 0.0f);
		glVertex2f(img_size.x, img_size.y);
		glVertex2f(0, img_size.y);
		glEnd();
	}

	if (image_tex) { //draw result
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_TRUE); //DEBUG
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, image_tex);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(  0.0f, 0.0f  ); glVertex2f(  0.0f,   0.0f);
		glTexCoord2f(img_size.x, 0.0f  ); glVertex2f(img_size.x,   0.0f);
		glTexCoord2f(img_size.x, img_size.y); glVertex2f(img_size.x, img_size.y);
		glTexCoord2f(  0.0f, img_size.y); glVertex2f(  0.0f, img_size.y);
		glEnd();
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
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

