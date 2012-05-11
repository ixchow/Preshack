#include "Module.hpp"

#include <ctype.h>
#include <cassert>

using std::cout;

map< string, ModuleCreator > &creators() {
	static map< string, ModuleCreator > ret;
	return ret;
}

map< string, string > &helps() {
	static map< string, string > ret;
	return ret;
}

map< string, ModuleCreator > const &get_module_map() {
	return creators();
}

string get_module_help(string const &name) {
	if (!helps().count(name)) return "";
	return helps()[name];
}

void register_module(string const &name, ModuleCreator creator, string const &help) {
	if (creators().count(name)) {
		cerr << "Name '" << name << "' is double-registered." << endl;
	} else {
		cout << "Registered module '" << name << "'" << endl;
		creators().insert(make_pair(name, creator));
		if (help != "") {
			assert(!helps().count(name));
			helps().insert(make_pair(name, help));
		}
	}
}

Module *create_module(std::string params) {
	string name = "";
	while (!params.empty() && !isspace(params[0])) {
		name += params[0];
		params.erase(0,1);
	}
	if (!params.empty()) {
		params.erase(0,1);
	}
	map< string, ModuleCreator >::iterator f = creators().find(name);
	if (f == creators().end()) {
		return NULL;
	}
	Module *ret = (f->second)(params);
	if (ret) {
		ret->params = name + " " + params;
	}
	return ret;
}

float Module::cursor_alpha() {
	return 1.0f;
}
