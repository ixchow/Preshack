#ifndef INTEGRATOR_HPP
#define INTEGRATOR_HPP

/*
 * Basic integrator that plugs into 'GeneralPaint' to actually make it tick.
 */

#include <string>

using std::string;

class GeneralPaintModule;

class Integrator {
public:
	virtual ~Integrator() {
	}
	//printed to let user know current integrator:
	virtual string name() = 0;
	//init from 'f' guess, store any viewable textures into 'viewable_textures':
	virtual void init(GeneralPaintModule *gp) = 0;
	//called every frame to actually do integrationings, possibly:
	virtual void update(GeneralPaintModule *gp) = 0;
	//after gradient changed:
	virtual void gradient_changed(GeneralPaintModule *gp) = 0;
};

#endif //INTEGRATOR_HPP
