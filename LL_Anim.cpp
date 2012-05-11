#include "LL_Anim.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/load_save_png.hpp>

#include <utility>
#include <cassert>

using std::swap;

#ifndef MACOSX
REQUIRE_GL_EXTENSION(GL_EXT_texture3);
#else
#define glTexImage3DEXT glTexImage3D
#endif //MACOSX

REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);
//REQUIRE_GL_EXTENSION(GL_ARB_multitexture);

namespace {
	Module *create_module(const std::string &params_in) {
		LayerAnim *m = new LayerAnim();
		if (params_in == "bars") {
			m->mode = BARS;
		} else if (params_in == "ball") {
			m->mode = BALL;
		} else if (params_in == "edges") {
			m->mode = EDGES;
		} else if (params_in == "stacking") {
			m->mode = STACKING;
		} else {
			std::cout << "Watch out! I know know the parameter of ths LayerAnim." << std::endl;
		}
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("ll_anim", create_module, "<bars|ball|edges|stacking>");
		}
	} fred;
}

const unsigned int Size = 128;
const unsigned int Len = 64;

namespace {
	float ang = -0.2f * M_PI;
	float elv = -0.2f * M_PI;
	bool show_flat = true;
	unsigned int frame = Len / 4;
	float tween = 0.0f;

	float fade_others = 1.0f;

	bool playing = false;
	float acc = 0.0f;

	bool needs_update = true;
	void global_update(float elapsed_time) {
		if (!needs_update) return;
		needs_update = false;
		if (show_flat) {
			tween -= elapsed_time;
			if (tween < 0.0f) tween = 0.0f;
		} else {
			tween += elapsed_time;
			if (tween > 1.0f) tween = 1.0f;
		}
		if (playing) {
			acc += elapsed_time;
			while (acc > 0.1f) {
				acc -= 0.1f;
				frame = (frame + 1) % Len;
			}
			fade_others -= elapsed_time;
			if (fade_others < 0.1f) fade_others = 0.1f;
		} else {
			fade_others += elapsed_time;
			if (fade_others > 1.0f) fade_others = 1.0f;
		}
	}
}

LayerAnim::LayerAnim() {
	mode = 0;
}

LayerAnim::~LayerAnim() {
}

Vector2f LayerAnim::size() {
	return make_vector(1.0f, 1.0f);
}

namespace {
	//dirs: +x, -x, +y, -y, +z, -z.
	vector< vector< Vector3f > > verts;
	vector< vector< Vector3f > > texcoords;
	GLuint ball_tex = 0;
	GLuint bars_tex = 0;
	GLuint tags_tex = 0;
	GLuint edges_tex = 0;

	vector< uint32_t > regions(Size*Size*Len);

	vector< vector< unsigned int > > lists;

	Graphics::ProgramObjectRef sel_shader = NULL;
	Graphics::ProgramObjectRef fade3d_shader = NULL;
	Graphics::ProgramObjectRef fade2d_shader = NULL;
}

void load_shaders() {
	{ //3dsel
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_3dsel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		sel_shader = Graphics::get_program_object(frag);
		assert(sel_shader.ref);
		glUseProgramObjectARB(sel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(sel_shader->handle, "tags"), 0);
		glUniform1iARB(glGetUniformLocationARB(sel_shader->handle, "ball"), 1);
		glUniform1iARB(glGetUniformLocationARB(sel_shader->handle, "bars"), 2);
		glUniform1iARB(glGetUniformLocationARB(sel_shader->handle, "sel"), 3);
		glUniform1iARB(glGetUniformLocationARB(sel_shader->handle, "fade"), 4);
		glUseProgramObjectARB(0);
	}
	{ //3dfade
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_3dfade.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		fade3d_shader = Graphics::get_program_object(frag);
		assert(fade3d_shader.ref);
		glUseProgramObjectARB(fade3d_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(fade3d_shader->handle, "tags"), 0);
		glUniform1iARB(glGetUniformLocationARB(fade3d_shader->handle, "ball"), 1);
		glUniform1iARB(glGetUniformLocationARB(fade3d_shader->handle, "bars"), 2);
		glUniform1iARB(glGetUniformLocationARB(fade3d_shader->handle, "sel"), 3);
		glUniform1iARB(glGetUniformLocationARB(fade3d_shader->handle, "fade"), 4);
		glUseProgramObjectARB(0);
	}
	{ //2dfade
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_2dfade.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		fade2d_shader = Graphics::get_program_object(frag);
		assert(fade2d_shader.ref);
		glUseProgramObjectARB(fade2d_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(fade2d_shader->handle, "tags"), 0);
		glUniform1iARB(glGetUniformLocationARB(fade2d_shader->handle, "ball"), 1);
		glUniform1iARB(glGetUniformLocationARB(fade2d_shader->handle, "bars"), 2);
		glUniform1iARB(glGetUniformLocationARB(fade2d_shader->handle, "sel"), 3);
		glUniform1iARB(glGetUniformLocationARB(fade2d_shader->handle, "fade"), 4);
		glUseProgramObjectARB(0);
	}
}

void make_textures() {
	vector< uint32_t > bars;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("ll_siggraph/anim-pillars.png", width, height, bars)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(bars.size() == Size * Size);
	}
	glGenTextures(1, &bars_tex);
	glBindTexture(GL_TEXTURE_2D, bars_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Size, Size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &bars[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	//------------------------------------
	vector< uint32_t > ball(Size*Size*Len);
	{
		unsigned int width = 0;
		unsigned int height = 0;
		vector< uint32_t > data;
		if (!load_png("ll_siggraph/anim-ball.png", width, height, data)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(data.size() == Size * Size);
		unsigned int min = Size;
		unsigned int max = 0;
		for (unsigned int y = 0; y < Size; ++y) {
			for (unsigned int x = 0; x < Size; ++x) {
				if (data[y * Size + x] >> 24) {
					if (min > x) min = x;
					if (max < x+1) max = x+1;
				}
			}
		}
		for (unsigned int t = 0; t < Len; ++t) {
			float amt = 0.5f * (1.0f - cosf(t / float(Len) * 2.0f * float(M_PI)));
			float ofs = amt * (Size - max - min);
			for (unsigned int y = 0; y < Size; ++y) {
				for (unsigned int x = 0; x < Size; ++x) {
					float px = x - ofs;
					int ix = (int)px;
					//float frac = px - ix;
					if (ix < 0) {
						ix = 0;
						//frac = 0.0f;
					}
					if ((unsigned)ix + 1 >= Size) {
						ix = Size - 1;
						//frac = 1.0f;
					}
					//TODO: actually lerp. I'm lazy.
					ball[t*Size*Size+y*Size+x] = data[y*Size+ix];
				}
			}
		}
	}
	glGenTextures(1, &ball_tex);
	glBindTexture(GL_TEXTURE_3D, ball_tex);
	glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA, Size, Size, Len, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ball[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_3D, 0);

	//---------------------------------------
	//Extract connected regions.
	for (unsigned int t = 0; t < Len; ++t) {
		for (unsigned int y = 0; y < Size; ++y) {
			for (unsigned int x = 0; x < Size; ++x) {
				uint32_t mask = 0xff000000;
				if (ball[t * Size * Size + y * Size + x] >> 24) {
					mask |= 1;
				}
				if (bars[y * Size + x] >> 24) {
					mask |= 2;
				}
				regions[t * Size * Size + y * Size + x] = mask;
			}
		}
	}
	{
		unsigned int tag = 0;
		Vector3ui start = make_vector(0U,0U,0U);
		while (1) {
			uint32_t v = regions[start.z * Size*Size + start.y * Size + start.x];
			if (v >> 24) {
				lists.push_back(vector< unsigned int >());
				if (v & 1) {
					lists.back().push_back(0);
				}
				if (v & 2) {
					lists.back().push_back(1);
				}
				vector< Vector3ui > to_explore;
				to_explore.push_back(start);
				regions[start.z * Size*Size + start.y * Size + start.x] = tag;
				while (!to_explore.empty()) {
					Vector3ui at = to_explore.back();
					to_explore.pop_back();
					const static Vector3i nbr[6] = {
						make_vector(-1,0,0),
						make_vector( 1,0,0),
						make_vector(0,-1,0),
						make_vector(0, 1,0),
						make_vector(0,0,-1),
						make_vector(0,0, 1),
					};
					for (unsigned int n = 0; n < 6; ++n) {
						Vector3ui next = at;
						next.x += nbr[n].x;
						next.y += nbr[n].y;
						next.z += nbr[n].z;
						if (next.x >= Size) continue;
						if (next.y >= Size) continue;
						if (next.z >= Len) continue;
						if (regions[next.z*Size*Size + next.y*Size + next.x] == v) {
							regions[next.z*Size*Size + next.y*Size + next.x] = tag;
							to_explore.push_back(next);
						}
					}
				}
				++tag;
			}
			start.x += 1;
			if (start.x >= Size) {
				start.y += 1;
				start.x = 0;
				if (start.y >= Size) {
					start.z += 1;
					start.y = 0;
					if (start.z >= Len) {
						break;
					}
				}
			}
		}
		//random-color tags:
		/*
		vector< uint32_t > tags(tag);
		for (unsigned int i = 0; i < tags.size(); ++i) {
			tags[i] = 0x88000000;
			tags[i] |= (rand() % 256) << 16;
			tags[i] |= (rand() % 256) << 8;
			tags[i] |= (rand() % 256);
		}
		tags[0] &= 0x00ffffff;
		for (unsigned int i = 0; i < regions.size(); ++i) {
			assert(regions[i] < tags.size());
			regions[i] = tags[regions[i]];
		}*/
	}
	glGenTextures(1, &tags_tex);
	glBindTexture(GL_TEXTURE_3D, tags_tex);
	glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA, Size, Size, Len, 0, GL_RGBA, GL_UNSIGNED_BYTE, &regions[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_3D, 0);

	vector< uint32_t > edges(Size*Size*Len);
	vector< uint32_t > col(lists.size());
	for (unsigned int i = 0; i < lists.size(); ++i) {
		if (lists[i].size() == 2) {
			col[i] = 0xbb000000 | (0xffffff & rand());
		} else if (lists[i].size() == 1) {
			col[i] = 0x11ffffff;
		} else {
			col[i] = 0;
		}
	}
	for (unsigned int t = 0; t < Len; ++t) {
		for (unsigned int y = 0; y < Size; ++y) {
			for (unsigned int x = 0; x < Size; ++x) {
				unsigned int i = t*Size*Size + y*Size + x;
				unsigned int p = regions[i];
				unsigned int p1 = p;
				unsigned int p2 = p;
				if (x+1 < Size) p1 = regions[i+1];
				if (y+1 < Size) p2 = regions[i+Size];
				if (p != p1 || p != p2) {
					edges[i] = 0x88000000;
				} else {
					edges[i] = col[regions[i]];
				}
			}
		}
	}
	glGenTextures(1, &edges_tex);
	glBindTexture(GL_TEXTURE_3D, edges_tex);
	glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA, Size, Size, Len, 0, GL_RGBA, GL_UNSIGNED_BYTE, &edges[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_3D, 0);

}


void LayerAnim::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	needs_update = true;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();

	if (ball_tex == 0 || bars_tex == 0 || tags_tex == 0) {
		make_textures();
		assert(ball_tex != 0);
		assert(bars_tex != 0);
		assert(tags_tex != 0);
	}
	if (!sel_shader.ref) {
		load_shaders();
	}
	if (verts.empty()) {
		verts.resize(6);
		for (unsigned int v = 0; v < Len; ++v) {
			float vv = (v + 0.5f) / float(Len) * 2.0f - 1.0f;
			const Vector2f dv[4] = {
				make_vector(-1.0f,-1.0f),
				make_vector(-1.0f, 1.0f),
				make_vector( 1.0f, 1.0f),
				make_vector( 1.0f,-1.0f),
			};
			for (unsigned int i = 0; i < 4; ++i) {
				verts[0].push_back(make_vector(vv,dv[i].x,dv[i].y));
				verts[1].push_back(make_vector(-vv,dv[i].x,dv[i].y));
				verts[2].push_back(make_vector(dv[i].x,vv,dv[i].y));
				verts[3].push_back(make_vector(dv[i].x,-vv,dv[i].y));
				verts[4].push_back(make_vector(dv[i].x,dv[i].y,vv));
				verts[5].push_back(make_vector(dv[i].x,dv[i].y,-vv));
			}
		}
	}


	unsigned int use = 0;
	glPushMatrix();
	{	//generate an ortho-ish matrix.

		float tang = tween * ang;
		float telv = tween * elv;

		float sang = sinf(tang);
		float cang = cosf(tang);
		float selv = sinf(telv);
		float celv = cosf(telv);

		Vector3f x_dir = make_vector(cang, sang*selv, sang*celv);
		Vector3f y_dir = make_vector(0.0f,celv,-selv);
		Vector3f z_dir = cross_product(x_dir, y_dir);
		float vals[6] = {
		/*
			 x_dir.z,
			-x_dir.z,
			 y_dir.z,
			-y_dir.z, */
			0.0f, 0.0f, 0.0f, 0.0f,
			 z_dir.z,
			-z_dir.z
		};
		use = 0;
		float amt = vals[0];
		for (unsigned int i = 1; i < 6; ++i) {
			if (vals[i] > amt) {
				amt = vals[i];
				use = i;
			}
		}
		x_dir.x *= 0.5f * size().x;
		y_dir.x *= 0.5f * size().x;
		z_dir.x *= 0.5f * size().x;
		x_dir.y *= 0.5f * size().y;
		y_dir.y *= 0.5f * size().y;
		z_dir.y *= 0.5f * size().y;
		float mat[16] = {
			x_dir.x, x_dir.y, 0.0f, 0.0f,
			y_dir.x, y_dir.y, 0.0f, 0.0f,
			z_dir.x, z_dir.y, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};
		glMultMatrixf(mat);
	}


	if (mode == EDGES || mode == BALL) {
		glUseProgramObjectARB(fade3d_shader->handle);
	} else if (mode == BARS) {
		glUseProgramObjectARB(fade2d_shader->handle);
	} else {
		glUseProgramObjectARB(sel_shader->handle);
	}

	glActiveTextureARB(GL_TEXTURE4);
	static GLuint fade_tex = 0;
	if (fade_tex == 0) {
		glGenTextures(1, &fade_tex);
	}
	glBindTexture(GL_TEXTURE_1D, fade_tex);
	{ //create texture for current stacking order:
		vector< uint32_t > fade(Len, 0x00000000);
		for (unsigned int f = 0; f < Len; ++f) {
			float d = (tween * tween) * 2.0f - fabsf((int)(f - frame) / float(Len));
			if (d > 1.0f) d = 1.0f;
			d *= fade_others;
			int amt = 256.0f * d;
			if (amt < 0) amt = 0;
			if (amt > 255) amt = 255;
			fade[f] = amt << 24;
		}
		fade[frame] = 0xff000000;
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, Len, 0, GL_RGBA, GL_UNSIGNED_BYTE, &fade[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	
	glActiveTextureARB(GL_TEXTURE3);
	static GLuint sel_tex = 0;
	if (sel_tex == 0) {
		glGenTextures(1, &sel_tex);
	}
	glBindTexture(GL_TEXTURE_1D, sel_tex);
	{ //create texture for current stacking order:
		vector< uint32_t > order(256, 0x00000000);
		assert(lists.size() <= 256);
		for (unsigned int i = 0; i < lists.size(); ++i) {
			order[i] = 0x000000ff;
			if (lists[i].empty()) {
				order[i] = 0x00ff0000;
			}
			if (lists[i].size() == 2 && lists[i][0] == 0) {
				order[i] = 0x0000ff00;
			}
		}
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, &order[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glActiveTextureARB(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bars_tex);

	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, ball_tex);

	glActiveTextureARB(GL_TEXTURE0);
	if (mode == EDGES) {
		glBindTexture(GL_TEXTURE_3D, edges_tex);
	} else if (mode == BALL) {
		glBindTexture(GL_TEXTURE_3D, ball_tex);
	} else {
		glBindTexture(GL_TEXTURE_3D, tags_tex);
	}

	Graphics::gl_errors("setup");

	glEnableClientState(GL_VERTEX_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	{
		float x[4] = {0.5f, 0.0f, 0.0f, 0.5f};
		float y[4] = {0.0f, 0.5f, 0.0f, 0.5f};
		float z[4] = {0.0f, 0.0f, 0.5f, 0.5f};
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenfv(GL_S, GL_OBJECT_PLANE, x);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenfv(GL_T, GL_OBJECT_PLANE, y);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
		glTexGenfv(GL_R, GL_OBJECT_PLANE, z);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);
	}


	glVertexPointer(3, GL_FLOAT, sizeof(Vector3f), &(verts[use][0]));


	glDrawArrays(GL_QUADS, 0, verts[use].size());

	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);

	glDisableClientState(GL_VERTEX_ARRAY);

	glActiveTextureARB(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_1D, 0);

	glActiveTextureARB(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_1D, 0);

	glActiveTextureARB(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTextureARB(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, 0);

	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, 0);
	glUseProgramObjectARB(0);

/*
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 1.0f, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 1.0f);
	glEnd();
*/

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	Graphics::gl_errors("LayerAnim::draw");

}
void LayerAnim::update(float elapsed_time) {
	global_update(elapsed_time);
}

bool LayerAnim::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		show_flat = !show_flat;
		return true;
	}

	if (mode == STACKING && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		unsigned int c = (int)((local_mouse.x / size().x + 0.5f) * Size);
		unsigned int r = (int)((local_mouse.y / size().y + 0.5f) * Size);
		if (r < Size && c < Size) {
			unsigned int tag = regions[frame*Size*Size + r*Size +c];
			if (tag < lists.size() && lists[tag].size() == 2) {
				swap(lists[tag][0], lists[tag][1]);
			}
		}

		return true;
	}
	static Vector2f old_local_mouse = make_vector(0.0f, 0.0f);
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
		old_local_mouse = local_mouse;
		return true;
	}
	if (event.type == SDL_MOUSEMOTION && !show_flat && event.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
		ang += -(local_mouse.x - old_local_mouse.x);
		elv += local_mouse.y - old_local_mouse.y;
		old_local_mouse = local_mouse;
		return true;
	}

	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_p) {
		playing = !playing;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAGEUP) {
		frame = (frame + 1) % Len;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAGEDOWN) {
		frame = (frame + Len - 1) % Len;
	}
	return false;
}

