#ifndef MODULE_HPP
#define MODULE_HPP

#include "Item.hpp"

#include <SDL.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>

using std::map;
using std::string;
using std::vector;
using std::cerr;
using std::endl;

class PortData;
class Connection;
typedef void (*PortDrawFn)(PortData const *, Connection const &, void *);

class PortData {
public:
	PortData() : position(make_vector(0.0f, 0.0f)), name("unnammed"), draw_fn(NULL), draw_fn_data(NULL) {
	}
	virtual ~PortData() {}
	virtual PortData *operator=(PortData const *other) = 0;
	virtual bool compatible(PortData const *other) = 0;
	virtual void reset() = 0; //reset to default value
	Vector2f position;
	const char *name;
	PortDrawFn draw_fn;
	void *draw_fn_data;
};

template < class FOO >
class PortDataContainer : public PortData {
public:
	virtual ~PortDataContainer() {}
	FOO foo;
	virtual PortData *operator=(PortData const *other) {
		PortDataContainer< FOO > const *other_cast = dynamic_cast< const PortDataContainer< FOO > * >(other);
		if (!other_cast) {
			cerr << "WARNING: trying to assign incompatible portdata." << endl;
		} else {
			foo = other_cast->foo;
		}
		return this;
	}
	virtual bool compatible(PortData const *other) {
		PortDataContainer< FOO > const *other_cast = dynamic_cast< const PortDataContainer< FOO > * >(other);
		return(other_cast != NULL);
	}
	virtual void reset() {
		/* no default by default */
	}
	FOO &operator()() {
		return foo;
	}
};

class Module : public Item {
public:
	Module() {
	}
	virtual ~Module() {}
	//Inhereted:
	//virtual Vector2f size();
	//virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//virtual bool get_native_res(Vector2ui const &into);
	virtual void update(float elapsed_time) = 0;
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse) = 0;
	virtual float cursor_alpha();
	vector< PortData * > in_ports;
	vector< PortData * > out_ports;
	std::string params; //creation params -- saved for file-saving.
};

typedef Module *(*ModuleCreator)(string const &);

void register_module(string const &name, ModuleCreator creator, string const &help = "");

Module *create_module(string params);

map< string, ModuleCreator > const &get_module_map();

string get_module_help(string const &name);


#define REGISTER_MODULE( NAME, CF, HELP ) \
namespace { \
	class RegModule { \
	public: \
		RegModule() { \
			register_module(NAME, CF, HELP); \
		} \
	} reg_module; \
}

#endif //MODULE_HPP
