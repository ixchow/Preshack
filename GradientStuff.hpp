#ifndef GRADIENTSTUFF_HPP
#define GRADIENTSTUFF_HPP 1

#include <Vector/Vector.hpp>
#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>

#include <vector>
#include <string>

#include <fftw3.h>

using std::vector;
using std::string;

//Helpers and defines for gradient painting:

void set_clamp_to_black();
void set_clamp_to_edge();
//void set_repeat(); //actually not allowed on texture_rectangle's.
void alloc_tex(unsigned int width, unsigned int height, GLuint &tex, vector< Vector3f > const &image = vector< Vector3f >());
void alloc_fb(GLuint tex, GLuint &fb);
void bind_fb(GLuint framebuffer, unsigned int width, unsigned int height);
void unbind_fb();

bool load_ppm(string filename, unsigned int &width, unsigned int &height, vector< float > &data);
bool load_ppm_channels(string filename, unsigned int &width, unsigned int &height, vector< float > &r, vector< float > &g, vector< float > &b);

//tweak the YCbCr shader:
void set_view_factors(float Y_scale = 1.0f, float Y_offset = 0.5f, float Cb_scale = 1.0f, float Cb_offset = 0.0f, float Cr_scale = 1.0f, float Cr_offset = 0.0f);

//Call this:
void init_gradient_stuff();

//To make all this valid:

//General stuff:
//constants used when doing RGB <-> YCbCr conversion.
const float Kb = 0.114;
const float Kr = 0.299;
//supply f; it outputs image in RGB:
extern Graphics::ProgramObjectRef YCbCr_shader;
//supply f, gx; it outputs residual:
extern Graphics::ProgramObjectRef gx_residual_shader;
//supply f, gy; it outputs residual:
extern Graphics::ProgramObjectRef gy_residual_shader;
//supply gx, gy; it outputs rhs:
extern Graphics::ProgramObjectRef rhs_shader;
//iteration: (supply f, rhs)
extern Graphics::ProgramObjectRef iter_shader;
//iteration, no corners: (supply f,rhs)
extern Graphics::ProgramObjectRef iter_shader_no_corners;
//for painting w/o color clamp controls:
extern Graphics::ProgramObjectRef texcoord_to_color;
//for band-limited-ish painting:
extern Graphics::ProgramObjectRef lf_shader; //gives low-freqencies of input tex
extern Graphics::ProgramObjectRef subtract_shader; //gives a-b
extern Graphics::ProgramObjectRef add_shader; //gives a+b (for the lazy)


//Stuff for simple plan:
const unsigned int SimpleWidth = 512;
const unsigned int SimpleHeight = 512;
extern float *simple_INV_FILTER; //deconvolve filter; (width) * (height/2+1)
//forward and backward plans for Width x Height r2c on simple_rhs & simple_RHS.
extern fftwf_plan simple_forward;
extern fftwf_plan simple_backward;
extern float *simple_rhs; //width x height
extern fftwf_complex *simple_RHS; //width x (height/2 + 1)


#endif //GRADIENTSTUFF_HPP
