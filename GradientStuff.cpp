#include "GradientStuff.hpp"

#include <Graphics/GLExtensions.hpp>

#include <iostream>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;

REQUIRE_GL_EXTENSION( GL_EXT_framebuffer_object )
REQUIRE_GL_EXTENSION( GL_ARB_texture_rectangle )
REQUIRE_GL_EXTENSION( GL_ARB_texture_float )
REQUEST_GL_EXTENSION( GL_ARB_color_buffer_float )
REQUIRE_GL_EXTENSION( GL_ARB_shader_objects )
REQUIRE_GL_EXTENSION( GL_ARB_multitexture )

void set_clamp_to_black() {
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);
	Graphics::gl_errors("set_clamp_to_black()");
}

void set_clamp_to_edge() {
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Graphics::gl_errors("set_clamp_to_edge()");
}

void alloc_tex(unsigned int width, unsigned int height, GLuint &tex, vector< Vector3f > const &image) {
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (image.size() != width * height) {
		if (!image.empty()) {
			cerr << "Image passed to alloc_tex has wrong size." << endl;
		}
		vector< Vector3f > blank(width * height, make_vector< float >(0,0,0));
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, width, height, 0, GL_RGB, GL_FLOAT, &(blank[0]));
	} else {
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB16F_ARB, width, height, 0, GL_RGB, GL_FLOAT, &(image[0]));
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc_tex");
}

void alloc_fb(GLuint tex, GLuint &fb) {
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);

	GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
		cerr << "FRAMEBUFFER NOT COMPLETE!" << endl;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	Graphics::gl_errors("alloc_fb");
}

void bind_fb(GLuint framebuffer, unsigned int width, unsigned int height) {
	glPushAttrib(GL_VIEWPORT_BIT);

	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(-1.0f, -1.0f, 0.0f);
	glScalef(2.0f / width, 2.0f / height, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
	Graphics::gl_errors("bind_fb");
}

void unbind_fb() {
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();
	Graphics::gl_errors("unbind_fb");
}

bool load_ppm(string filename, unsigned int &width, unsigned int &height, vector< float > &data) {
	ifstream im(filename.c_str());
	char p, six;
	unsigned int maxval;
	if (!(im >> p >> six >> width >> height >> maxval)) {
		cerr << "Error reading '" << filename << "'" << endl;
		return false;
	}
	if (p != 'P' || six != '6') {
		cerr << "Error reading '" << filename << "': bad magic." << endl;
		return false;
	}
	if (width * height > 100000000) {
		cerr << "File '" << filename << "' fails size sanity cheack." << endl;
		return false;
	}
	if (maxval > 255) {
		cerr << "File '" << filename << "' has larger-than-byte maxval." << endl;
		return false;
	}
	char ws;
	if (!(im.read(&ws, 1))) {
		cerr << "Error reading single whitespace byte before raster." << endl;
		return false;
	}
	vector< unsigned char > pixdata(width*height*3);
	if (!im.read((char *)&(pixdata[0]), pixdata.size())) {
		cerr << "Error reading raster from file." << endl;
		return false;
	}
	data.resize(width*height*3);
	float inv = 1.0f / maxval;
	for (unsigned int i = 0; i < data.size(); ++i) {
		data[i] = pixdata[i] * inv;
	}
	return true;
}

bool load_ppm_channels(string filename, unsigned int &width, unsigned int &height, vector< float > &r, vector< float > &g, vector< float > &b) {
	vector< float > data;
	if (!load_ppm(filename, width, height, data)) {
		return false;
	}
	assert(width * height * 3 == data.size());
	r.resize(width * height);
	g.resize(width * height);
	b.resize(width * height);
	for (unsigned int i = 0; i < r.size(); ++i) {
		r[i] = data[i*3+0];
		g[i] = data[i*3+1];
		b[i] = data[i*3+2];
	}
	return true;
}

void set_view_factors(float Y_scale, float Y_offset, float Cb_scale, float Cb_offset, float Cr_scale, float Cr_offset) {
	Vector4f rfactor = make_vector(Y_scale, 0.0f, Cr_scale * 2.0f * (1.0f - Kr), 0.0f);
	Vector4f bfactor = make_vector(Y_scale, Cb_scale * 2.0f * (1.0f - Kr), 0.0f, 0.0f);
	Vector4f gfactor = make_vector(Y_scale, 0.0f, 0.0f, 0.0f);

	gfactor -= Kr * rfactor;
	gfactor -= Kb * bfactor;
	gfactor /= (1 - Kr - Kb);

	rfactor.w += make_vector(Y_offset, Cb_offset, Cr_offset) * rfactor.xyz;
	gfactor.w += make_vector(Y_offset, Cb_offset, Cr_offset) * gfactor.xyz;
	bfactor.w += make_vector(Y_offset, Cb_offset, Cr_offset) * bfactor.xyz;

	if (YCbCr_shader.ref) {
		glUniform4fARB(glGetUniformLocationARB(YCbCr_shader->handle, "rfactor"), rfactor.x, rfactor.y, rfactor.z, rfactor.w);
		glUniform4fARB(glGetUniformLocationARB(YCbCr_shader->handle, "gfactor"), gfactor.x, gfactor.y, gfactor.z, gfactor.w);
		glUniform4fARB(glGetUniformLocationARB(YCbCr_shader->handle, "bfactor"), bfactor.x, bfactor.y, bfactor.z, bfactor.w);
	} else {
		cerr << "not setting view factors" << endl;
	}
	Graphics::gl_errors("Set view factors");
}

//General stuff:
//constants used when doing RGB <-> YCbCr conversion.
Graphics::ProgramObjectRef YCbCr_shader = NULL;
Graphics::ProgramObjectRef gx_residual_shader = NULL;
Graphics::ProgramObjectRef gy_residual_shader = NULL;
Graphics::ProgramObjectRef rhs_shader = NULL;
Graphics::ProgramObjectRef iter_shader = NULL;
Graphics::ProgramObjectRef iter_shader_no_corners = NULL;
Graphics::ProgramObjectRef texcoord_to_color = NULL;
Graphics::ProgramObjectRef lf_shader = NULL;
Graphics::ProgramObjectRef subtract_shader = NULL;
Graphics::ProgramObjectRef add_shader = NULL;


//Stuff for simple plan:
float *simple_INV_FILTER = NULL;
fftwf_plan simple_forward = NULL;
fftwf_plan simple_backward = NULL;
float *simple_rhs = NULL;
fftwf_complex *simple_RHS = NULL;

void init_gradient_stuff() {
	static bool inited = false;
	if (inited) return;
	inited = true;
	cout << "Initializing Gradient Stuff." << endl;

	//-------------------------------------------------------------
	//Load shaders:

	{ //YCbCr_shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/ycbcr_view.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		YCbCr_shader = Graphics::get_program_object(frag);
		assert(YCbCr_shader.ref);
		glUseProgramObjectARB(YCbCr_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(YCbCr_shader->handle, "image"), 0);
		set_view_factors();
		glUseProgramObjectARB(0);
	}

	{ //gx_residual_shader
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/gx_residual.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		gx_residual_shader = Graphics::get_program_object(frag);
		assert(gx_residual_shader.ref);
		glUseProgramObjectARB(gx_residual_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(gx_residual_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(gx_residual_shader->handle, "gx"), 1);
		glUseProgramObjectARB(0);
	}

	{ //gy_residual_shader
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/gy_residual.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		gy_residual_shader = Graphics::get_program_object(frag);
		assert(gy_residual_shader.ref);
		glUseProgramObjectARB(gy_residual_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(gy_residual_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(gy_residual_shader->handle, "gy"), 1);
		glUseProgramObjectARB(0);
	}

	{ //rhs_shader
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/rhs.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		rhs_shader = Graphics::get_program_object(frag);
		assert(rhs_shader.ref);
		glUseProgramObjectARB(rhs_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gx"), 0);
		glUniform1iARB(glGetUniformLocationARB(rhs_shader->handle, "gy"), 1);
		glUseProgramObjectARB(0);
	}

	{ //iter_shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/iter_shader.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		iter_shader = Graphics::get_program_object(frag);
		assert(iter_shader.ref);
		glUseProgramObjectARB(iter_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(iter_shader->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}

	{ //iter_shader_no_corners:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/iter_shader_no_corners.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		iter_shader_no_corners = Graphics::get_program_object(frag);
		assert(iter_shader_no_corners.ref);
		glUseProgramObjectARB(iter_shader_no_corners->handle);
		glUniform1iARB(glGetUniformLocationARB(iter_shader_no_corners->handle, "f"), 0);
		glUniform1iARB(glGetUniformLocationARB(iter_shader_no_corners->handle, "rhs"), 1);
		glUseProgramObjectARB(0);
	}

	{ //texcoord_to_color:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/texcoord_to_color.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		texcoord_to_color = Graphics::get_program_object(frag);
		assert(texcoord_to_color.ref);
	}

	{ //lf_shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/lf.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		lf_shader = Graphics::get_program_object(frag);
		assert(lf_shader.ref);
		glUseProgramObjectARB(lf_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(lf_shader->handle, "tex"), 0);
		glUseProgramObjectARB(0);
	}

	{ //subtract_shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/subtract.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		subtract_shader = Graphics::get_program_object(frag);
		assert(subtract_shader.ref);
		glUseProgramObjectARB(subtract_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(subtract_shader->handle, "a"), 0);
		glUniform1iARB(glGetUniformLocationARB(subtract_shader->handle, "b"), 1);
		glUseProgramObjectARB(0);
	}
	{ //add_shader:
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("gradient/shaders/add.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		add_shader = Graphics::get_program_object(frag);
		assert(add_shader.ref);
		glUseProgramObjectARB(add_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(add_shader->handle, "a"), 0);
		glUniform1iARB(glGetUniformLocationARB(add_shader->handle, "b"), 1);
		glUseProgramObjectARB(0);
	}





	//-------------------------------------------------------------
	//Set up for Simple FFT stuff:

	simple_INV_FILTER = (float *)fftwf_malloc(sizeof(float) * SimpleWidth * SimpleHeight);
	simple_rhs = (float *)fftwf_malloc(sizeof(float)*SimpleWidth*SimpleHeight);
	simple_RHS = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*SimpleWidth*(SimpleHeight/2+1));
	//plan some transforms:
	simple_forward = fftwf_plan_dft_r2c_2d(SimpleWidth, SimpleHeight, simple_rhs, simple_RHS, FFTW_MEASURE);
	simple_backward = fftwf_plan_dft_c2r_2d(SimpleWidth, SimpleHeight, simple_RHS, simple_rhs, FFTW_MEASURE);

	{ //calculate simple_INV_FILTER:
		float *filter = simple_rhs;
		fftwf_complex *FILTER = simple_RHS; //just use this memory.

		for (unsigned int i = 0; i < SimpleWidth * SimpleHeight; ++i) {
			filter[i] = 0.0f;
		}
		filter[0*SimpleWidth+0] = -4.0f;
		filter[0*SimpleWidth+1] =  1.0f;
		filter[1*SimpleWidth+0] =  1.0f;
		filter[0*SimpleWidth+SimpleWidth-1] = 1.0f;
		filter[(SimpleHeight-1)*SimpleWidth+0] = 1.0f;
		fftwf_execute(simple_forward);
		float energy = 0.0f;
		float fac = 1.0f / (SimpleWidth * SimpleHeight);
		for (unsigned int i = 0; i < SimpleWidth * (SimpleHeight/2+1); ++i) {
			energy += fabsf(FILTER[i][1]);
			if (FILTER[i][0] != 0.0f) {
				simple_INV_FILTER[i] = 1.0f / FILTER[i][0] * fac;
			} else {
				simple_INV_FILTER[i] = 0.0f;
			}
		}
	}
}
