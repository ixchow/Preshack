#ifndef VCYCLE_HPP
#define VCYCLE_HPP

#include "GP_Integrator.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Vector/Vector.hpp>

#include <vector>

using std::vector;

class VLevel {
public:
	VLevel() : f_tex(0), f_fb(0), f_temp_tex(0), f_temp_fb(0), rhs_tex(0), rhs_fb(0) { }
	//level of solution; want to solve:
	// stencil * F = RHS
	//describe stencil:
	float middle, edge, corner, magic_xA, magic_xB;
	GLuint f_tex;
	GLuint f_fb;
	GLuint f_temp_tex;
	GLuint f_temp_fb;
	GLuint rhs_tex;
	GLuint rhs_fb;
	unsigned int width;
	unsigned int height;
};

class ALevel {
public:
	//levels used for centering average.
	unsigned int width;
	unsigned int height;
	GLuint tex;
	GLuint fb;
};

/*
 * This is a standard multigrid VCycle, as per NRC.
 *
 */
class VCycle : public Integrator {
public:
	VCycle(Vector3f _grey_point = make_vector(0.0f, 0.0f, 0.0f)) : grey_point(_grey_point) {
	}
	virtual ~VCycle() {
	}
	virtual string name() {
		return "VCycle";
	}
	virtual void init(GeneralPaintModule *gp);
	virtual void update(GeneralPaintModule *gp);
	virtual void gradient_changed(GeneralPaintModule *gp);
	void dump_operator(GeneralPaintModule *gp);

	vector< VLevel > levels;

	vector< ALevel > avg;

	Vector3f grey_point;

	Graphics::ProgramObjectRef rhs_shader; //build initial RHS
	Graphics::ProgramObjectRef residual_shader; //compute residual
	Graphics::ProgramObjectRef restrict_shader; //restrict from fine to coarse
	Graphics::ProgramObjectRef interpolate_add_shader; //interpolate from coarse to fine (and add to old version)
	Graphics::ProgramObjectRef interpolate_shader; //interpolate from coarse to fine (no old version required)
	Graphics::ProgramObjectRef iter_shader; //step toward solution

	Graphics::ProgramObjectRef recenter_scale_shader; //null shader to avoid clamping. used when recentering.
	Graphics::ProgramObjectRef recenter_sub_shader; //subtract value in 1px image from final rendering.
};

#endif //VCYCLEINTEGRATOR_HPP
