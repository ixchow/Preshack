#include "SS_Volume.hpp"

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
		SoftVolume *m = new SoftVolume();
		return m;
	}
	class Fred {
	public:
		Fred() {
			register_module("ss_volume", create_module, "");
		}
	} fred;
}

const unsigned int Size = 128;
const unsigned int Len = 64;

namespace {
	float ang = -0.2f * M_PI;
	float elv = -0.2f * M_PI;
	bool show_flat = true;
	float tween = 0.0f;

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
	}
}

SoftVolume::SoftVolume() {
}

SoftVolume::~SoftVolume() {
}

Vector2f SoftVolume::size() {
	return make_vector(1.0f, 1.0f);
}

namespace {
	//dirs: +x, -x, +y, -y, +z, -z.
	vector< vector< Vector3f > > verts;
	vector< vector< Vector3f > > texcoords;
	GLuint vol_tex = 0;
}


namespace {
union Col {
	uint32_t val;
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
};

void make_textures() {
	vector< uint32_t > fog_front;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("defense/mountain-vol-sm.ff.png", width, height, fog_front)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(fog_front.size() == Size * Size);
	}
	vector< uint32_t > fog_side;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("defense/mountain-vol-sm.fs.png", width, height, fog_side)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(fog_side.size() == Size * Size);
	}
	vector< uint32_t > fog_top;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("defense/mountain-vol-sm.ft.png", width, height, fog_top)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(fog_top.size() == Size * Size);
	}
	vector< uint32_t > mountain;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("defense/mountain-vol-sm.m.png", width, height, mountain)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(mountain.size() == Size * Size);
	}
	vector< uint32_t > depth;
	{
		unsigned int width = 0;
		unsigned int height = 0;
		if (!load_png("defense/mountain-vol-sm.md.png", width, height, depth)) {
			assert(0);
		}
		assert(width == Size);
		assert(height == Size);
		assert(depth.size() == Size * Size);
	}
	vector< uint32_t > vol(Size * Size * Len);
	vector< float > acc_trans(Size * Size, 1.0f);
	for (unsigned int l = Len - 1; l < Len; --l) {
		for (unsigned int px = 0; px < Size * Size; ++px) {
			Col fc;
			{
				Col front; front.val = fog_front[px];
				Col top; top.val = fog_top[l * Size / Len * Size + px % Size];
				Col side; side.val = fog_side[px / Size * Size + l * Size / Len];
				fc.r = ((int)side.r + (int)front.r + (int)top.r) / 3;
				fc.g = ((int)side.g + (int)front.g + (int)top.g) / 3;
				fc.b = ((int)side.b + (int)front.b + (int)top.b) / 3;
				fc.a = ((int)side.a * (int)front.a * (int)top.a) / 255 / 255;
			}
			/*
			float target_trans = 1.0f - fc.a / 255.0f;
			//Want: acc_trans * (1.0f - alpha) == target_trans
			if (target_trans >= acc_trans[px]) {
				fc.a = 0; //nothing for it.
			} else {
				int amt = (1.0f - target_trans / acc_trans[px]) * 255.0f + 0.5f;
				if (amt > 255) amt = 255;
				if (amt < 0) amt = 0;
				fc.a = amt;
			}*/

			Col c = fc;
			Col mc; mc.val = mountain[px];
			unsigned int d = depth[px] & 0xff;
			if (l * 256 / Len <= d && (l+1) * 256 / Len > d) {
				if (mc.a) {
					c = mc;
				}
			} else {
				acc_trans[px] *= (1.0f - fc.a / 255.0f);
			}
			vol[l * Size * Size + px] = c.val;
		}
	}

	glGenTextures(1, &vol_tex);
	glBindTexture(GL_TEXTURE_3D, vol_tex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA, Size, Size, Len, 0, GL_RGBA, GL_UNSIGNED_BYTE, &vol[0]);
	glBindTexture(GL_TEXTURE_3D, 0);
	Graphics::gl_errors("make textures");
}
}


void SoftVolume::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {
	needs_update = true;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	/*
	glBegin(GL_QUADS);
	glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
	glVertex2f(-size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f, -size().y*0.5f);
	glVertex2f( size().x*0.5f,  size().y*0.5f);
	glVertex2f(-size().x*0.5f,  size().y*0.5f);
	glEnd();
	*/

	if (vol_tex == 0) {
		make_textures();
		assert(vol_tex != 0);
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
		
			 x_dir.z,
			-x_dir.z,
			 y_dir.z,
			-y_dir.z, 
			//0.0f, 0.0f, 0.0f, 0.0f,
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


	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, vol_tex);
	glEnable(GL_TEXTURE_3D);

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

	glActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, 0);

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	Graphics::gl_errors("SoftVolume::draw");

}
void SoftVolume::update(float elapsed_time) {
	global_update(elapsed_time);
}

bool SoftVolume::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		show_flat = !show_flat;
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
	return false;
}

