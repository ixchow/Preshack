#include "GP_VCycle.hpp"
#include "GP_GeneralPaintModule.hpp"
#undef ERROR
#define ERROR( X ) std::cerr << X << std::endl
#define WARNING( X ) std::cout << X << std::endl
#define INFO( X ) std::cout << X << std::endl

#include <sstream>
#include <fstream>

using std::ostringstream;
using std::ofstream;

REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_ARB_multitexture )

#include "gp_gl_helpers.hpp"


using std::swap;
using std::cout;
using std::endl;

using std::map;
using std::pair;
using std::make_pair;

namespace {
	map< pair< pair< unsigned int, unsigned int >, bool >, pair< GLuint, GLuint > > temp_textures;
	void make_temp_texture(unsigned int width, unsigned int height, int kind, GLuint &tex, GLuint &fb) {
		pair< pair< unsigned int, unsigned int >, int > adr = make_pair(make_pair(width, height), kind);
		auto f = temp_textures.find(adr);
		if (f != temp_textures.end()) {
			tex = f->second.first;
			fb = f->second.second;
		} else {
			alloc_tex(width, height, tex);
			alloc_fb(tex, fb);
			temp_textures.insert(make_pair(adr, make_pair(tex, fb)));
			INFO("Allocating " << width << "x" << height << " texture of kind " << kind << ".");
		}
	}
};

void VCycle::init(GeneralPaintModule *gp) {
	assert(levels.empty()); //avoid double-init.

	//allocate levels:
	unsigned int cur_width, cur_height;
	cur_width = gp->ImageWidth;
	cur_height = gp->ImageHeight;
	while (1) {
		levels.push_back(VLevel());
		VLevel &lev = levels.back();
		{ //coefs:
			float s = (1 << (levels.size()-1));
			float m = -8*s*s-4;
			float e = s*s + 2;
			float c = s*s - 1;
			m /= 3*s*s;
			e /= 3*s*s;
			c /= 3*s*s;
			lev.corner = c;
			lev.edge   = e;
			lev.middle = m;
			//TODO: some sort of verification?
			//TODO: This is not an informed choice yet:
			//These are from 33-size optimization:
			if (levels.size() == 1) {
				lev.magic_xA =-1.1142; //2.5091;
				lev.magic_xB = 3.1077; //-0.1218f;
			} else if (levels.size() == 2) {
				lev.magic_xA =-1.8957; //-1.4635f;
				lev.magic_xB = 0.8905; //0.9103f;
			} else if (levels.size() == 3) {
				lev.magic_xA =-2.1868; //0.3111f;
				lev.magic_xB = 0.5785; //-2.4975f;
			} else if (levels.size() == 4) {
				lev.magic_xA =-1.5088; //-0.7212f;
				lev.magic_xB = 0.7585; // 0.1142f;
			} else if (levels.size() == 5) {
				lev.magic_xA =-2.2388;
				lev.magic_xB = 0.7193;
			} else {
				lev.magic_xA =-0.9638;//-0.9582;
				lev.magic_xB = 0.9952;// 1.0491;
			}
		}
		lev.width = cur_width;
		lev.height = cur_height;

		if (levels.size() == 1) {
			//the first level. Tricky.
			lev.f_tex = gp->f_tex;
			lev.f_fb = gp->f_fb;

			alloc_tex(cur_width, cur_height, lev.rhs_tex);
			alloc_fb(lev.rhs_tex, lev.rhs_fb);

			//alloc_tex(cur_width, cur_height, lev.f_temp_tex);
			//alloc_fb(lev.f_temp_tex, lev.f_temp_fb);
			//I (think) we can do this...
			make_temp_texture(cur_width, cur_height, 1, lev.f_temp_tex, lev.f_temp_fb);

		} else if (levels.size() > 1) {
			make_temp_texture(cur_width, cur_height, 0, lev.f_tex, lev.f_fb);
			make_temp_texture(cur_width, cur_height, 1, lev.f_temp_tex, lev.f_temp_fb);
			make_temp_texture(cur_width, cur_height, 2, lev.rhs_tex, lev.rhs_fb);
		} else {
			assert(0);
		}

		ostringstream name;
		name << levels.size() << ": " << cur_width << "x" << cur_height;
		gp->viewable_textures.push_back( ViewableTexture(lev.f_tex, lev.f_fb, cur_width, cur_height, "f" + name.str()) );
		gp->viewable_textures.push_back( ViewableTexture(lev.rhs_tex, lev.rhs_fb, cur_width, cur_height, "rhs" + name.str()) );

		if (cur_width == 1 && cur_height == 1) break;
		cur_width = ((cur_width - 1) / 2) + 1;
		cur_height = ((cur_height - 1) / 2) + 1;
	}

	unsigned int awidth = 64;
	unsigned int aheight = 64;
	while (awidth > gp->ImageWidth) awidth >>= 1;
	while (aheight > gp->ImageHeight) aheight >>= 1;

	while (1) {
		avg.push_back(ALevel());
		ALevel &lev = avg.back();
		lev.width = awidth;
		lev.height = aheight;

		//alloc_tex(awidth, aheight, lev.tex);
		//alloc_fb(lev.tex, lev.fb);
		make_temp_texture(awidth, aheight, 10, lev.tex, lev.fb);

		if (awidth == 1 && aheight == 1) break;

		if (awidth > 1) awidth >>= 1;
		if (aheight > 1) aheight >>= 1;
	}

	/* Ignore for now:
	//---------------------------------
	for (unsigned int l = 0; l < f_levels.size(); ++l) {
		coefs.push_back(Coefs());
		float s = (1 << l);
		float m = -8*s*s-4;
		float e = s*s + 2;
		float c = s*s - 1;
		m /= 3*s*s;
		e /= 3*s*s;
		c /= 3*s*s;
		coefs.back().corner = c;
		coefs.back().edge   = e;
		coefs.back().center = m;
	} //for (level)
	*/

	//---------------------------------
	{ //rhs from gx,gy:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/mg_rhs.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		rhs_shader = Graphics::get_program_object(frag);
		assert(rhs_shader.ref);
		glUseProgramObjectARB(rhs_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gx"), 0);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gy"), 1);
		glUseProgramObjectARB(0);
	}
	{ //residual:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_residual.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		residual_shader = Graphics::get_program_object(frag);
		assert(residual_shader.ref);
		glUseProgramObjectARB(residual_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(residual_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(residual_shader->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}
	{ //restrict:
		#ifdef ATI_HACK
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_restrict_ATI_HACK.glsl", GL_FRAGMENT_SHADER_ARB);
		#else
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_restrict.glsl", GL_FRAGMENT_SHADER_ARB);
		#endif
		assert(frag.ref);
		restrict_shader = Graphics::get_program_object(frag);
		assert(restrict_shader.ref);
		glUseProgramObjectARB(restrict_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(restrict_shader->handle, "img"), 0);
		glUseProgramObjectARB(0);
	}
	{ //interpolate:
		#ifdef ATI_HACK
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate_add_ATI_HACK.glsl", GL_FRAGMENT_SHADER_ARB);
		#else
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate_add.glsl", GL_FRAGMENT_SHADER_ARB);
		#endif
		assert(frag.ref);
		interpolate_add_shader = Graphics::get_program_object(frag);
		assert(interpolate_add_shader.ref);
		glUseProgramObjectARB(interpolate_add_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(interpolate_add_shader->handle, "small"), 0);
		glUniform1iARB(glGetUniformLocationARB(interpolate_add_shader->handle, "add"), 1);
		glUseProgramObjectARB(0);
	}
	{ //interpolate:
		#ifdef ATI_HACK
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate_ATI_HACK.glsl", GL_FRAGMENT_SHADER_ARB);
		#else
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_interpolate.glsl", GL_FRAGMENT_SHADER_ARB);
		#endif
		assert(frag.ref);
		interpolate_shader = Graphics::get_program_object(frag);
		assert(interpolate_shader.ref);
		glUseProgramObjectARB(interpolate_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(interpolate_shader->handle, "small"), 0);
		glUseProgramObjectARB(0);
	}

	{ //iterate:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_iter.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		iter_shader = Graphics::get_program_object(frag);
		assert(iter_shader.ref);
		glUseProgramObjectARB(iter_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}

	{ //recenter scale shader:
		#ifdef ATI_HACK
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_recenter_scale_ATI_HACK.glsl", GL_FRAGMENT_SHADER_ARB);
		#else
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_recenter_scale.glsl", GL_FRAGMENT_SHADER_ARB);
		#endif
		assert(frag.ref);
		recenter_scale_shader = Graphics::get_program_object(frag);
		assert(recenter_scale_shader.ref);
		glUseProgramObjectARB(recenter_scale_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(recenter_scale_shader->handle, "img"), 0);
		glUseProgramObjectARB(0);
	}
	{ //recenter subtract shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gp_shaders/vc_recenter_sub.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		recenter_sub_shader = Graphics::get_program_object(frag);
		assert(recenter_sub_shader.ref);
		glUseProgramObjectARB(recenter_sub_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(recenter_sub_shader->handle, "img"), 0);
		glUniform1iARB(glGetUniformLocationARB(recenter_sub_shader->handle, "sub"), 1);
		glUseProgramObjectARB(0);
	}




}

namespace { //helpers

#define RESTRICT_PROJECT_CLAMP set_clamp_to_edge

void smooth(Graphics::ProgramObjectRef &iter_shader, VLevel &lev, float magic_x) {
	bind_fb(lev.f_temp_fb, lev.width, lev.height);

	glUseProgramObjectARB(iter_shader->handle);

	//Set up shader for this level:
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_corner"), lev.corner);
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_edge"), lev.edge);
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_x"), magic_x);
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_center_minus_x_inv"), 1.0f / (lev.middle-magic_x));

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, lev.rhs_tex);
	set_nearest();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, lev.f_tex);
	set_clamp_to_edge(); set_nearest();

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(0,0);
	glTexCoord2f(lev.width,0); glVertex2f(lev.width,0);
	glTexCoord2f(lev.width,lev.height); glVertex2f(lev.width,lev.height);
	glTexCoord2f(0,lev.height); glVertex2f(0,lev.height);
	glEnd();

	//TODO: BAD BAD BAD! Try something else!
	/*
	//TEST:
	//remove some energy from the lower right:
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_corner"), 0.5 * lev.corner);
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_edge"), 0.5 * lev.edge);
	glUniform1fARB(glGetUniformLocationARB(iter_shader->handle, "w_center_inv"), 0.5f / lev.middle);

	glBegin(GL_QUADS);
	glTexCoord2f(lev.width-1,0); glVertex2f(lev.width-1,0);
	glTexCoord2f(lev.width,0); glVertex2f(lev.width,0);
	glTexCoord2f(lev.width,lev.height); glVertex2f(lev.width,lev.height);
	glTexCoord2f(lev.width-1,lev.height); glVertex2f(lev.width-1,lev.height);
	glEnd();
	*/

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();

	swap(lev.f_temp_fb, lev.f_fb);
	swap(lev.f_temp_tex, lev.f_tex);

	Graphics::gl_errors("smooth");
}

//Drat! 'restrict' is a keyword!
void coarsify(Graphics::ProgramObjectRef &residual_shader, Graphics::ProgramObjectRef &restrict_shader, bool first_level, VLevel &lev, VLevel &to) {

	if (first_level) {
	//a) calculate residual into 'f_temp':
	bind_fb(lev.f_temp_fb, lev.width, lev.height);

	glUseProgramObjectARB(residual_shader->handle);

	//Set up shader for this level:
	glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_corner"), lev.corner);
	glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_edge"), lev.edge);
	glUniform1fARB(glGetUniformLocationARB(residual_shader->handle, "w_center"), lev.middle);

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, lev.rhs_tex);
	set_nearest();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, lev.f_tex);
	set_clamp_to_edge(); set_nearest();

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(0,0);
	glTexCoord2f(lev.width,0); glVertex2f(lev.width,0);
	glTexCoord2f(lev.width,lev.height); glVertex2f(lev.width,lev.height);
	glTexCoord2f(0,lev.height); glVertex2f(0,lev.height);
	glEnd();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();
	} //end if(first_level)

	//b) restrict 'f_temp' into the next level:
	bind_fb(to.rhs_fb, to.width, to.height);

	glUseProgramObjectARB(restrict_shader->handle);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, first_level?lev.f_temp_tex:lev.rhs_tex);
	//RESTRICT_PROJECT_CLAMP(); //whatever clamp that is
	set_clamp_to_black(); //otherwise left, top edges blow up.
	#ifdef ATI_HACK
	set_nearest();
	#else
	set_linear(); //no ATI_HACK
	#endif

	glBegin(GL_QUADS);
	glTexCoord2f(-0.5f, -0.5f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(2*to.width-0.5f, -0.5f); glVertex2f(to.width, 0.0f);
	glTexCoord2f(2*to.width-0.5f,2*to.height-0.5f); glVertex2f(to.width, to.height);
	glTexCoord2f(-0.5f,2*to.height-0.5f); glVertex2f(0.0f, to.height);
	glEnd();

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();

/* will get over-written anyway
	//c) guess is zero residual:
	bind_fb(to.f_fb, to.width, to.height);
	glClear(GL_COLOR_BUFFER_BIT);
	unbind_fb(); */

	Graphics::gl_errors("coarsify/restrict");
}

void interpolate(Graphics::ProgramObjectRef &interpolate_shader, Graphics::ProgramObjectRef &interpolate_add_shader, bool first_level, VLevel &lev, VLevel &from) {
	glDisable(GL_BLEND);

	bind_fb(lev.f_temp_fb, lev.width, lev.height);

	if (first_level) {
		glUseProgramObjectARB(interpolate_add_shader->handle);
	} else {
		glUseProgramObjectARB(interpolate_shader->handle);
	}

	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, lev.f_tex);
	set_nearest();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, from.f_tex);
	RESTRICT_PROJECT_CLAMP(); //whatever clamp that is
	#ifdef ATI_HACK
	set_nearest();
	#else
	set_linear(); //no ATI_HACK
	#endif


	glBegin(GL_QUADS);
	glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f );
	glTexCoord2f(0.25f, 0.25f);
	glVertex2f(0.0f, 0.0f);

	glMultiTexCoord2f(GL_TEXTURE1, lev.width, 0.0f );
	glTexCoord2f(0.25f+0.5f*lev.width, 0.25f);
	glVertex2f(lev.width, 0.0f);

	glMultiTexCoord2f(GL_TEXTURE1, lev.width, lev.height );
	glTexCoord2f(0.25f+0.5f*lev.width, 0.25f+0.5f*lev.height);
	glVertex2f(lev.width, lev.height);

	glMultiTexCoord2f(GL_TEXTURE1, 0.0f, lev.height );
	glTexCoord2f(0.25f, 0.25f+0.5f*lev.height);
	glVertex2f(0.0f, lev.height);
	glEnd();

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glUseProgramObjectARB(0);

	unbind_fb();

	swap(lev.f_temp_fb, lev.f_fb);
	swap(lev.f_temp_tex, lev.f_tex);

	Graphics::gl_errors("interpolate");
}

}//end anon helper namespace


void VCycle::update(GeneralPaintModule *gp) {
	glDisable(GL_BLEND); //just to be sure.

	//hack (should really just not allocate one of these):
	//DEBUG:
	//swap(gp->f_tex, levels[0].f_tex);
	//swap(gp->f_fb, levels[0].f_fb);

	//down:
	for (unsigned int l = 0; l + 1 < levels.size(); ++l) {
		//pre-smooth:
		//smooth(iter_shader, levels[l]); //pre-smoothing seems to cause problems.
		//restrict:
		coarsify(residual_shader, restrict_shader, l == 0, levels[l], levels[l+1]);
	}

	bind_fb(levels.back().f_fb, levels.back().width, levels.back().height);
	glClear(GL_COLOR_BUFFER_BIT);
	unbind_fb();

	//up:
	for (unsigned int l = levels.size() - 2; l < levels.size(); --l) {
		//interpolate:
		interpolate(interpolate_shader, interpolate_add_shader, l == 0, levels[l], levels[l+1]);
		//post-smooth:
		smooth(iter_shader, levels[l], levels[l].magic_xA);
		smooth(iter_shader, levels[l], levels[l].magic_xB);
	}

	//recenter:
	//a) first come up with an 'average color':
	for (unsigned int a = 0; a < avg.size(); ++a) {
		ALevel &lev = avg[a];
		bind_fb(lev.fb, lev.width, lev.height);

		glUseProgramObjectARB(recenter_scale_shader->handle);

		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		unsigned int tw, th;
		if (a == 0) {
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, levels[0].f_tex);
			tw = levels[0].width;
			th = levels[0].height;
		} else {
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, avg[a-1].tex);
			tw = avg[a-1].width;
			th = avg[a-1].height;
		}
		set_clamp_to_edge();
		#ifdef ATI_HACK
		set_nearest();
		#else
		set_linear(); //no ATI_HACK
		#endif

		glBegin(GL_QUADS);
		glTexCoord2f(0,0);   glVertex2f(0, 0);
		glTexCoord2f(tw,0);  glVertex2f(lev.width, 0);
		glTexCoord2f(tw,th); glVertex2f(lev.width, lev.height);
		glTexCoord2f(0,th);  glVertex2f(0, lev.height);
		glEnd();

		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);

		unbind_fb();
	}

	{//b) actually recenter:
		bind_fb(levels[0].f_temp_fb, levels[0].width, levels[0].height);
		glUseProgramObjectARB(recenter_sub_shader->handle);
		glUniform3fARB(glGetUniformLocationARB(recenter_sub_shader->handle, "grey"), grey_point.x, grey_point.y, grey_point.z);


		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, avg.back().tex);
		set_nearest(); set_clamp_to_edge();

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, levels[0].f_tex);
		set_nearest();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(0, 0);
		glTexCoord2f(levels[0].width, 0); glVertex2f(levels[0].width, 0);
		glTexCoord2f(levels[0].width, levels[0].height); glVertex2f(levels[0].width, levels[0].height);
		glTexCoord2f(0, levels[0].height); glVertex2f(0, levels[0].height);
		glEnd();

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		glUseProgramObjectARB(0);
		unbind_fb();

		swap(levels[0].f_tex, levels[0].f_temp_tex);
		swap(levels[0].f_fb, levels[0].f_temp_fb);
	} //done recentering.


	for (unsigned int l = 0; l < levels.size(); ++l) {
		gp->viewable_textures.at(gp->viewable_textures.size() - levels.size() * 2 + l * 2).tex = levels[l].f_tex;
		gp->viewable_textures.at(gp->viewable_textures.size() - levels.size() * 2 + l * 2).fb = levels[l].f_fb;
	}

	//(LAME) way of getting image over to generalpaint:
	gp->f_tex = levels.at(0).f_tex;
	gp->f_fb = levels.at(0).f_fb;
	//(there is a viewable-texture-modifying hack that supports this)

	//hack, reprise (see above):
	//DEBUG:
	//swap(gp->f_tex, levels[0].f_tex);
	//swap(gp->f_fb, levels[0].f_fb);
	//

}

void VCycle::gradient_changed(GeneralPaintModule *gp) {
	glDisable(GL_BLEND); //be sure.

	//calculate-ify various RHS:
	
	bind_fb(levels[0].rhs_fb, gp->ImageWidth, gp->ImageHeight);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gp->gx_tex);
	set_clamp_to_black();
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gp->gy_tex);
	set_clamp_to_black();

	glUseProgramObjectARB(rhs_shader->handle);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex2f(0,0);
	glTexCoord2f(gp->ImageWidth,0); glVertex2f(gp->ImageWidth,0);
	glTexCoord2f(gp->ImageWidth,gp->ImageHeight); glVertex2f(gp->ImageWidth,gp->ImageHeight);
	glTexCoord2f(0,gp->ImageHeight); glVertex2f(0,gp->ImageHeight);
	glEnd();

	glUseProgramObjectARB(0);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	unbind_fb();

	Graphics::gl_errors("VCycle::gradient_changed");
}

void VCycle::dump_operator(GeneralPaintModule *gp) {
	glDisable(GL_BLEND); //to be sure.

	if (gp->ImageHeight * gp->ImageWidth > 5000) {
		ERROR("I'm not going to dump an operator this big; sorry.");
		return;
	}

	ostringstream file;
	file << "operator." << gp->ImageHeight << "x" << gp->ImageWidth << ".dump";
	INFO("Dumpping " << file.str());
	ofstream out(file.str().c_str());

	for (unsigned int pix_y = 0; pix_y < gp->ImageHeight; ++pix_y) {
		for (unsigned int pix_x = 0; pix_x < gp->ImageWidth; ++pix_x) {
			//set up rhs:
			bind_fb(levels[0].rhs_fb, gp->ImageWidth, gp->ImageHeight);
			glClear(GL_COLOR_BUFFER_BIT);
			glColor3f(1.0f, 1.0f, 1.0f);
			glBegin(GL_POINTS);
			glVertex2f(pix_x + 0.5f, pix_y + 0.5f);
			glEnd();
			unbind_fb();
			//set up f:
			bind_fb(levels[0].f_fb, gp->ImageWidth, gp->ImageHeight);
			glClear(GL_COLOR_BUFFER_BIT);
			unbind_fb();

			Graphics::gl_errors("Construct");

			//so at this point the residual is one only at (pix_x, pix_y).

			//Single step (Copied from above w/ recentering stuff removed):

			//down (DUMP):
			for (unsigned int l = 0; l + 1 < levels.size(); ++l) {
				//pre-smooth:
				//smooth(iter_shader, levels[l]); //pre-smoothing seems to cause problems.
				//restrict:
				coarsify(residual_shader, restrict_shader, l == 0, levels[l], levels[l+1]);
			}

			bind_fb(levels.back().f_fb, levels.back().width, levels.back().height);
			glClear(GL_COLOR_BUFFER_BIT);
			unbind_fb();

			//up (DUMP):
			for (unsigned int l = levels.size() - 2; l < levels.size(); --l) {
				//interpolate:
				interpolate(interpolate_shader, interpolate_add_shader, l == 0, levels[l], levels[l+1]);
				//post-smooth:
				smooth(iter_shader, levels[l], levels[l].magic_xA);
				smooth(iter_shader, levels[l], levels[l].magic_xB);
			}

			//So now levels[0].f gives us the present correction, which is what we will dump:
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, levels[0].f_tex);
			static vector< Vector3f > tex;
			if (tex.size() != gp->ImageWidth * gp->ImageHeight) {
				tex.resize(gp->ImageWidth * gp->ImageHeight);
			}
			glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, GL_FLOAT, &(tex[0]));
			for (unsigned int i = 0; i < tex.size(); ++i) {
				if (i != 0)	out << ' ';
				out << tex[i].x;
			}
			out << '\n';
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

			Graphics::gl_errors("Dump");
		}
	}
}

