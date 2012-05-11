#include "PresHack.hpp"

#include "String.hpp"
#include "Image.hpp"
#include "Equation.hpp"

#include "HQ.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/load_save_png.hpp>
#include <Vector/VectorGL.hpp>
#include <Basis/ListDir.hpp>
#include <Basis/Error.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <limits>
#include <algorithm>

#include <ctype.h>

#include "shapes.hpp"

REQUEST_GL_EXTENSION(GL_EXT_framebuffer_object);
REQUEST_GL_EXTENSION(GL_ARB_shader_objects);
//REQUIRE_GL_EXTENSION(GL_ARB_multitexture);

bool edit_mode = false;

using std::cout;
using std::cerr;
using std::endl;

using std::ifstream;
using std::ofstream;
using std::getline;

using std::istringstream;
using std::ostringstream;

using std::make_pair;
using std::pair;

using std::set;

using std::numeric_limits;
using std::reverse;


PresHack::PresHack() {
	clear();
}

PresHack::~PresHack() {
	clear();
}

//Helper:
string get_string(Item *item) {
	if (dynamic_cast< String * >(item)) {
		return dynamic_cast< String * >(item)->text;
	} else if (dynamic_cast< Image * >(item)) {
		return dynamic_cast< Image * >(item)->path;
	} else if (dynamic_cast< Equation * >(item)) {
		return dynamic_cast< Equation * >(item)->equation;
	} else if (dynamic_cast< Module * >(item)) {
		return dynamic_cast< Module * >(item)->params;
	} else {
		cout << "WARNING: Unhandled 'get_string' item..." << endl;
		return "";
	}
}

void PresHack::save(string const &filename) {
	ofstream file(filename.c_str());
	for (auto i = items.begin(); i != items.end(); ++i) {
		if (dynamic_cast< Frame * >(i->second)) {
			Frame *f = dynamic_cast< Frame * >(i->second);
			file << "frame " << f->id << endl;
			file << "\ttop_color " << f->top_color << endl;
			file << "\tbottom_color " << f->bottom_color << endl;
			file << "\tarea " << f->area.min << " " << f->area.max << endl;
			file << "\tscreen_height " << f->screen_height << endl;
			file << "\tscreen_center " << f->screen_center << endl;
			for (auto p = f->previews.begin(); p != f->previews.end(); ++p) {
				file << "\tpreview " << p->min << " " << p->max << endl;
			}
			for (auto p = f->fixes.begin(); p != f->fixes.end(); ++p) {
				file << "\tfix " << *p << endl;
			}
			for (auto c = f->items.begin(); c != f->items.end(); ++c) {
				if (*c == NULL) continue;
				file << "\tnest " << (*c)->id << " in " << f->id << endl;
			}
			for (auto c = f->connections.begin(); c != f->connections.end(); ++c) {
				file << "\tconnect " << c->from_module->id << " " << c->from_port << " to " << c->to_module->id << " " << c->to_port;
				if (c->have_start) file << " start " << c->start;
				if (c->have_end) file << " end " << c->end;
				if (c->points.size()) {
					file << " points";
					for (unsigned int p = 0; p < c->points.size(); ++p) {
						file << " " << c->points[p];
					}
				}
				file << endl;
			}
		} else if (dynamic_cast< String * >(i->second)) {
			String *s = dynamic_cast< String * >(i->second);
			file << "string " << s->id << " " << s->text << endl;
			file << "\tcolor " << s->color << endl;
		} else if (dynamic_cast< Image * >(i->second)) {
			Image *im = dynamic_cast< Image * >(i->second);
			file << "image " << im->id << " " << im->path << endl;
		} else if (dynamic_cast< Equation * >(i->second)) {
			Equation *eq = dynamic_cast< Equation * >(i->second);
			file << "equation " << eq->id << " " << eq->equation << endl;
		} else if (dynamic_cast< Module * >(i->second)) {
			Module *m = dynamic_cast< Module * >(i->second);
			file << "module " << m->id << " " << m->params << endl;
		} else {
			cerr << "Unrecognized item." << endl;
			continue;
		}
		file << "\tposition " << (i->second)->position << endl;
		file << "\tscale " << (i->second)->scale << endl;
	}
	for (unsigned int sn = 0; sn < snaps.size(); ++sn) {
		Snap const &snap = snaps[sn];
		file << "snap";
		for (unsigned int s = 0; s < snap.stack.size(); ++s) {
			file << " " << snap.stack[s]->id;
		}
		file << " " << snap.current->id << endl;
		for (auto s = snap.screens.begin(); s != snap.screens.end(); ++s) {
			file << "\tscreen " << s->first->id << " " << s->second.center << " " << s->second.height << endl;
		}
	}
	if (current) {
		file << "stack";
		for (unsigned int s = 0; s < stack.size(); ++s) {
			file << " " << stack[s]->id;
		}
		file << " " << current->id;
		file << endl;
	}
}

class LoadedSnap {
public:
	vector< ItemId > stack;
	vector< pair< ItemId, Screen > > screens;
};

class LoadedCon {
public:
	LoadedCon() :
		frame((unsigned)-1),
		from_module((unsigned)-1), to_module((unsigned)-1),
		from_port((unsigned)-1), to_port((unsigned)-1),
		have_start(false), start(make_vector(0.0f, 0.0f)),
		have_end(false), end(make_vector(0.0f, 0.0f)) {
	}
	ItemId frame;
	ItemId from_module;
	ItemId to_module;
	unsigned int from_port;
	unsigned int to_port;
	bool have_start;
	Vector2f start;
	bool have_end;
	Vector2f end;
	vector< Vector2f > points;
};

bool PresHack::load(string const &filename) {
	clear();

	ifstream file(filename.c_str());
	string line;
	Item *last = NULL;

	vector< Item * > loaded;
	vector< pair< ItemId, ItemId > > nests;
	vector< LoadedCon > connections;
	vector< ItemId > loaded_stack;

	vector< LoadedSnap > loaded_snaps;

	while (getline(file, line)) {
		//first-character '#' => comment.
		if (!line.empty() && line[0] == '#') line = "";

		istringstream line_str(line);
		string tok;
		if (!(line_str >> tok) || tok == "") continue;
		if (tok == "nest") {
			ItemId a,b;
			string temp;
			if (!(line_str >> a >> temp >> b) || temp != "in" ) {
				cerr << "Expecting a nest 'a in b' got '" << line << "'." << endl;
				continue;
			}
			nests.push_back(make_pair(a,b));
		} else if (tok == "frame") {
			Frame *frame = new Frame();
			frame->previews.clear(); //expecting some preview later...
			if (!current) current = frame;
			last = frame;
			if (!(line_str >> last->id)) {
				cerr << "expecting id in '" << line << "'" << endl;
			}
			loaded.push_back(last);
		} else if (tok == "string") {
			String *s = new String();
			last = s;
			if (!(line_str >> last->id)) {
				cerr << "expecting id in '" << line << "'" << endl;
			}
			while (isspace(line_str.peek())) line_str.get();
			s->text = "";
			while (line_str.peek() != EOF) s->text += (char)line_str.get();
			loaded.push_back(last);
		} else if (tok == "image") {
			Image *im = new Image();
			last = im;
			if (!(line_str >> last->id)) {
				cerr << "expecting id in '" << line << "'" << endl;
			}
			while (isspace(line_str.peek())) line_str.get();
			im->path = "";
			while (line_str.peek() != EOF) im->path += (char)line_str.get();
			loaded.push_back(last);
		} else if (tok == "equation") {
			Equation *eq = new Equation();
			last = eq;
			if (!(line_str >> last->id)) {
				cerr << "expecting id in '" << line << "'" << endl;
			}
			while (isspace(line_str.peek())) line_str.get();
			eq->equation = "";
			while (line_str.peek() != EOF) eq->equation += (char)line_str.get();
			loaded.push_back(last);
		} else if (tok == "module") {
			ItemId id = 0;
			if (!(line_str >> id)) {
				cerr << "expecting id in '" << line << "'" << endl;
			}
			while (isspace(line_str.peek())) line_str.get();
			string params = "";
			while (line_str.peek() != EOF) params += (char)line_str.get();
			Module *mod = create_module(params);
			if (!mod) {
				cerr << "Cannot create module from '" << params << "'" << endl;
			} else {
				last = mod;
				mod->id = id;
				loaded.push_back(last);
			}
		} else if (tok == "position") {
			if (!last) {
				cerr << "No last, but got a position." << endl;
				continue;
			}
			if (!(line_str >> last->position)) {
				cerr << "expecting position in '" << line << "'" << endl;
			}
		} else if (tok == "scale") {
			if (!last) {
				cerr << "No last, but got a scale." << endl;
				continue;
			}
			if (!(line_str >> last->scale)) {
				cerr << "expecting scale in '" << line << "'" << endl;
			}
		} else if (tok == "color") {
			String *s = dynamic_cast< String * >(last);
			if (!s) {
				cerr << "Expecting a string for 'color'" << endl;
				continue;
			}
			if (!(line_str >> s->color)) {
				cerr << "Expect string color." << endl;
				continue;
			}
		} else if (tok == "area") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'area'" << endl;
				continue;
			}
			if (!(line_str >> frame->area.min >> frame->area.max)) {
				cerr << "Expect frame area." << endl;
				continue;
			}
		} else if (tok == "preview") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'area'" << endl;
				continue;
			}
			Box2f temp;
			if (!(line_str >> temp.min >> temp.max)) {
				cerr << "Expect frame area." << endl;
				continue;
			}
			if (frame->previews.empty()) {
				frame->preview = temp;
			}
			frame->previews.push_back(temp);
		} else if (tok == "fix") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'fix'" << endl;
				continue;
			}
			Vector2f temp;
			if (!(line_str >> temp)) {
				cerr << "Expect fix vector." << endl;
				continue;
			}
			frame->fixes.push_back(temp);
		} else if (tok == "top_color") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'top_color'" << endl;
				continue;
			}
			if (!(line_str >> frame->top_color)) {
				cerr << "Expect top color." << endl;
				continue;
			}
		} else if (tok == "bottom_color") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'bottom_color'" << endl;
				continue;
			}
			if (!(line_str >> frame->bottom_color)) {
				cerr << "Expect bottom color." << endl;
				continue;
			}
		} else if (tok == "screen_height") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'screen_height'" << endl;
				continue;
			}
			if (!(line_str >> frame->screen_height)) {
				cerr << "Expect screen height." << endl;
				continue;
			}
		} else if (tok == "screen_center") {
			Frame *frame = dynamic_cast< Frame * >(last);
			if (!frame) {
				cerr << "Expecting a frame for 'screen_center'" << endl;
				continue;
			}
			if (!(line_str >> frame->screen_center)) {
				cerr << "Expect screen center." << endl;
				continue;
			}
		} else if (tok == "stack") {
			loaded_stack.clear();
			ItemId temp;
			while (line_str >> temp) {
				loaded_stack.push_back(temp);
			}
		} else if (tok == "snap") {
			loaded_snaps.push_back(LoadedSnap());
			ItemId temp;
			while (line_str >> temp) {
				loaded_snaps.back().stack.push_back(temp);
			}
			if (loaded_snaps.back().stack.empty()) {
				loaded_snaps.pop_back();
				cerr << "Expected at least one stack element in snap." << endl;
				continue;
			}
		} else if (tok == "screen") {
			if (!loaded_snaps.size()) {
				cerr << "Screen without a snap." << endl;
				continue;
			}
			ItemId id;
			Screen temp;
			if (!(line_str >> id >> temp.center >> temp.height)) {
				cerr << "Expecting an id, center, and height for screen." << endl;
				continue;
			}
			loaded_snaps.back().screens.push_back(make_pair(id, temp));
		} else if (tok == "connect") {
			LoadedCon temp;
			if (!last) {
				cerr << "connect outside of a frame." << endl;
				continue;
			}
			temp.frame = last->id;
			if (!(line_str >> temp.from_module >> temp.from_port >> tok >> temp.to_module >> temp.to_port) || tok != "to") {
				cout << "Expecting connect id port to id port" << endl;
				continue;
			}
			while (line_str >> tok) {
				if (tok == "start") {
					if (line_str >> temp.start) {
						temp.have_start = true;
					} else {
						cout << "Expecting point after 'start' token." << endl;
					}
				} else if (tok == "end") {
					if (line_str >> temp.end) {
						temp.have_end = true;
					} else {
						cout << "Expecting point after 'end' token." << endl;
					}
				} else if (tok == "points") {
					Vector2f vec;
					while (line_str >> vec) {
						temp.points.push_back(vec);
					}
				} else {
					cout << "Strange token in connect line '" << tok << "'" << endl;
				}
			}
			connections.push_back(temp);
		} else {
			cerr << "I don't understand '" << tok << "'" << endl;
		}
	}

	for (auto i = loaded.begin(); i != loaded.end(); ++i) {
		while (items.count((*i)->id)) {
			cerr << "Id collision on '" << (*i)->id << "'" << endl;
			(*i)->id = rand();
		}
		items.insert(make_pair((*i)->id, *i));
	}
	map< ItemId, Frame * > has_parent;
	for (auto n = nests.begin(); n != nests.end(); ++n) {
		Frame *f = NULL;
		Item *i = NULL;
		if (has_parent.count(n->first)) {
			cout << "Trying to nest an item [" << n->first << "] which already has a parent!" << endl;
			continue;
		}
		if (items.count(n->first)) {
			i = items[n->first];
		} else {
			cerr << "Missing id '" << n->first << "'" << endl;
			continue;
		}
		if (items.count(n->second) && dynamic_cast< Frame * >(items[n->second])) {
			f = dynamic_cast< Frame * >(items[n->second]);
		} else {
			cerr << "Missing or not frame id '" << n->second << "'" << endl;
			continue;
		}
		has_parent.insert(make_pair(n->first, f));
		f->items.push_back(i);
		if (dynamic_cast< Module * >(i)) {
			f->modules.push_back(dynamic_cast< Module * >(i));
		}
	}

	if (!loaded_stack.empty()) {
		for (auto s = loaded_stack.begin(); s != loaded_stack.end(); ++s) {
			if (items.count(*s) && dynamic_cast< Frame * >(items[*s])) {
				stack.push_back(dynamic_cast< Frame * >(items[*s]));
			} else {
				cerr << "Item '" << *s << "' isn't an id or frame." << endl;
			}
		}
		if (!stack.empty()) {
			current = stack.back();
			stack.pop_back();
		}
		for (unsigned int s = 0; s < stack.size(); ++s) {
			stack[s]->tween_in = 1.0f;
			stack[s]->tween = 1.0f;
			if (s + 1 < stack.size()) {
				stack[s]->tween_target = stack[s+1];
			} else {
				stack[s]->tween_target = current;
			}
		}
		current->tween_in = 1.0f;
		current->tween = 0.0f;
	}

	for (auto c = connections.begin(); c != connections.end(); ++c) {
		Frame *f = NULL;
		if (items.count(c->frame)) {
			f = dynamic_cast< Frame * >(items[c->frame]);
		}

		if (!f) {
			cerr << "Connection in non-existent frame." << endl;
			continue;
		}

		Module *m_from = NULL;
		if (items.count(c->from_module)) {
			m_from = dynamic_cast< Module * >(items[c->from_module]);
		}
		if (!m_from) {
			cout << "Missing from module in connection." << endl;
			continue;
		}

		Module *m_to = NULL;
		if (items.count(c->to_module)) {
			m_to = dynamic_cast< Module * >(items[c->to_module]);
		}
		if (!m_to) {
			cout << "Missing to module in connection." << endl;
			continue;
		}

		if (m_from->out_ports.size() <= c->from_port) {
			cout << "Invalid from port in connection." << endl;
			continue;
		}

		if (m_to->in_ports.size() <= c->to_port) {
			cout << "Invalid to port in connection." << endl;
			continue;
		}

		f->connections.push_back(Connection());
		f->connections.back().from_module = m_from;
		f->connections.back().to_module = m_to;
		f->connections.back().from_port = c->from_port;
		f->connections.back().to_port = c->to_port;
		f->connections.back().have_start = c->have_start;
		f->connections.back().have_end = c->have_end;
		f->connections.back().start = c->start;
		f->connections.back().end = c->end;
		f->connections.back().points = c->points;
	}

	for (unsigned int sn = 0; sn < loaded_snaps.size(); ++sn) {
		snaps.push_back(Snap());
		LoadedSnap const &loaded_snap = loaded_snaps[sn];
		Snap &snap = snaps.back();
		for (vector< ItemId >::const_iterator s = loaded_snap.stack.begin(); s != loaded_snap.stack.end(); ++s) {
			if (items.count(*s) && dynamic_cast< Frame * >(items[*s])) {
				snap.stack.push_back(dynamic_cast< Frame * >(items[*s]));
			} else {
				cerr << "Item '" << *s << "' isn't an id or frame." << endl;
			}
		}
		if (snap.stack.empty()) {
			cerr << "No valid stack in loaded snap." << endl;
			snaps.pop_back();
			continue;
		}
		snap.current = snap.stack.back();
		snap.stack.pop_back();
		for (auto s = loaded_snap.screens.begin(); s != loaded_snap.screens.end(); ++s) {
			if (items.count(s->first) && dynamic_cast< Frame * >(items[s->first])) {
				if (snap.screens.count(dynamic_cast< Frame * >(items[s->first]))) {
					cerr << "Double-specification for screen." << endl;
				} else {
					snap.screens.insert(make_pair(dynamic_cast< Frame * >(items[s->first]), s->second));
				}
			} else {
				cerr << "Item '" << s->first << "' isn't an id or frame." << endl;
			}
		}
	}

	//check for missing parents:
	for (auto i = items.begin(); i != items.end();) {
		auto old = i;
		++i;
		if (!has_parent.count((old->second)->id)) {
			if (!stack.empty()) {
				if (old->second == stack[0]) continue;
			} else {
				if (old->second == current) continue;
			}
			cerr << "Removing item with id [" << (old->second)->id << "], which has no parent." << endl;
			items.erase(old);
		} 
	}

	//Freshen ids:
	ItemId fresh = 0;
	auto old_items = items;
	items.clear();
	for (auto i = old_items.begin(); i != old_items.end(); ++i) {
		i->second->id = fresh;
		items.insert(make_pair(i->second->id, i->second));
		++fresh;
	}

	if (current) {
		current->tween_in = 1.0f;
	} else {
		cerr << "No current frame!" << endl;
	}

	//pre-cache-draw:
	for (auto i = items.begin(); i != items.end(); ++i) {
		Box2f all;
		all.min = -0.5f * i->second->size();
		all.max =  0.5f * i->second->size();
		if (dynamic_cast< Frame * >(i->second)) {
			dynamic_cast< Frame * >(i->second)->update_modules(0.01f);
			dynamic_cast< Frame * >(i->second)->update_modules(0.01f);
			dynamic_cast< Frame * >(i->second)->update_modules(0.01f);
		}
		i->second->draw(all, all, 1.0f, 0);
	}

	return true; //optimistic!
}

void PresHack::clear() {
	for (auto i = items.begin(); i != items.end(); ++i) {
		//delete i->second; //DEBUG.
		cerr << "Leaking memory! But it's probably ok..." << endl;
		break;
	}
	items.clear();
	current = NULL;
	tween = TWEEN_NONE;
	show = SHOW_CURRENT;
	edit_mode = false;
	edit_point = NULL;
	edit_scale = NULL;
	edit_string = NULL;
	edit_string_cursor = 0;
	fix_target = (unsigned)-1;
	scrolling = false;
	zooming = false;
	drawing = false;
	boxing = false;
	mouse = make_vector(0.0f, 0.0f);
	mouse_locked = 0;
	mouse_vel = make_vector(0.0f, 0.0f, 0.0f);

	snaps.clear();
	target_snap = (unsigned)-1;
	previous_snap.stack.clear();
	previous_snap.current = NULL;
	previous_snap.screens.clear();
	snap_tween = 0.0f;
	snap_tweening = false;

	source_module = NULL;
	source_port = 0;
}

void PresHack::copy(ostream &out) {
	if (!current) return;
	Box2f internal;
	{
		Box2f trash;
		current->get_boxes(internal, trash);
	}
	float scale = 1.0f / internal.size().y;
	int at = 0;
	map< Module *, int > have;
	for (vector< Item * >::iterator i = current->items.begin(); i != current->items.end(); ++i) {
		Box2f extents = make_box((*i)->position - 0.5f * (*i)->size() * (*i)->scale, (*i)->position + 0.5f * (*i)->size() * (*i)->scale);
		if (!extents.intersects(internal)) continue;
		if (dynamic_cast< Frame * >(*i)) continue;
		out << "make " << ((*i)->position - internal.min) * scale << " " << (*i)->scale * scale << " ";
		if (dynamic_cast< String * >(*i)) {
			out << "." << dynamic_cast< String * >(*i)->text;
		} else if (dynamic_cast< Image * >(*i)) {
			out << ":" << dynamic_cast< Image * >(*i)->path;
		} else if (dynamic_cast< Equation * >(*i)) {
			out << ";" << dynamic_cast< Equation * >(*i)->equation;
		} else if (dynamic_cast< Module * >(*i)) {
			Module *m = dynamic_cast< Module * >(*i);
			out << "+" << m->params;
			have.insert(make_pair(m, at));
			++at;
		} else {
			cout << "Odd, not expecting something else." << endl;
		}
		out << "\n";
	}
	--at;
	for (vector< Connection >::iterator c = current->connections.begin(); c != current->connections.end(); ++c) {
		if (!have.count(c->from_module)) continue;
		if (!have.count(c->to_module)) continue;
		out << "con " << have[c->from_module] - at << ":" <<  c->from_port << " > " << have[c->to_module] - at << ":" << c->to_port;
		if (c->have_start) {
			out << " start: " << c->start;
		}
		if (c->have_end) {
			out << " end: " << c->end;
		}
		if (c->points.size()) {
			out << " via: ";
			for (unsigned int p = 0; p < c->points.size(); ++p) {
				out << " " << (c->points[p] - internal.min) * scale;
			}
		}
		out << "\n";
	}
}

void PresHack::paste(istream &in) {
	if (!current) return;
	Box2f internal;
	{
		Box2f trash;
		current->get_boxes(internal, trash);
	}
	float scale = internal.size().y;

	string line;
	vector< Module * > have;
	while (getline(in, line)) {
		istringstream line_str(line);
		string tok;
		if (!(line_str >> tok)) continue;
		if (tok == "make") {
			Vector2f pos;
			float scl;
			char type;
			if (!(line_str >> pos >> scl >> type)) {
				cerr << "Error on line '" << line << "'." << endl;
				continue;
			}
			string rest = "";
			char temp;
			while (line_str.read(&temp, 1)) {
				rest += temp;
			}
			Item *created = NULL;
			if (type == ':') {
				Image *image = new Image;
				image->path = rest;
				created = image;
			} else if (type == ';') {
				Equation *equation = new Equation(rest);
				created = equation;
			} else if (type == '+') {
				Module *module = create_module(rest);
				created = module;
				if (module) {
					have.push_back(module);
					current->modules.push_back(module);
				}
			} else if (type == '.') {
				String *str = new String;
				str->text = rest;
				created = str;
			}
			if (created) {
				created->position = pos * scale + internal.min;
				created->scale = scl * scale;
				current->items.push_back(created);
				items.insert(make_pair(created->id, created));
				}
			} else if (tok == "con") {
				char ex_colon1, ex_gt, ex_colon2;
				int from, to;
				unsigned int from_port, to_port;
				if (!(line_str >> from >> ex_colon1 >> from_port >> ex_gt >> to >> ex_colon2 >> to_port) || ex_colon1 != ':' || ex_colon2 != ':' || ex_gt != '>' ) {
					cerr << "Error on line '" << line << "'." << endl;
					continue;
				}
				Connection con;
				if ((unsigned)(have.size() - 1 + from) >= have.size()) {
					cerr << "Bad from idx." << endl;
					continue;
				} else {
					con.from_module = have[have.size() - 1 + from];
				}
				if ((unsigned)(have.size() - 1 + to) >= have.size()) {
					cerr << "Bad to idx." << endl;
					continue;
				} else {
					con.to_module = have[have.size() - 1 + to];
				}
				if (from_port >= con.from_module->out_ports.size()) {
					cerr << "bad from port." << endl;
					continue;
				} else {
					con.from_port = from_port;
				}
				if (to_port >= con.to_module->in_ports.size()) {
					cerr << "bad to port." << endl;
					continue;
				} else {
					con.to_port = to_port;
				}
				string tok;
				while (line_str >> tok) {
					if (tok == "(") {
						line_str.putback(tok[0]);
						Vector2f pt;
						if (!(line_str >> pt)) {
							cerr << "Weird; expecting point.";
						} else {
							con.points.push_back(pt * scale + internal.min);
						}
					} else if (tok == "start:") {
						Vector2f pt;
						if (!(line_str >> pt)) {
							cerr << "Weird; expecting start point.";
						} else {
							con.start = pt;
							con.have_start = true;
						}
					} else if (tok == "end:") {
						Vector2f pt;
						if (!(line_str >> pt)) {
							cerr << "Weird; expecting end point.";
						} else {
							con.end = pt;
							con.have_end = true;
						}
					} else if (tok == "via:") {
						//do nothing.
					} else {
						cerr << "Unexpected tok in points list: '" << tok << "'." << endl;
					}
				}
				current->connections.push_back(con);
			}
		}
	}
	
	void PresHack::store_snap(Snap &target, Snap const &rel) {
		target.stack = stack;
		target.current = current;
		target.screens.clear();
		for (auto s = rel.screens.begin(); s != rel.screens.end(); ++s) {
			Screen temp;
			temp.height = s->first->screen_height;
			temp.center = s->first->screen_center;
			target.screens.insert(make_pair(s->first, temp));
		}
	}
	
	void PresHack::handle_event(SDL_Event const &event) {
		if (event.type == SDL_MOUSEMOTION /*&& (tween == TWEEN_NONE)*/) {
			mouse.x = (event.motion.x / float(Graphics::screen_x) * 2.0f - 1.0f) * Graphics::aspectf();
			mouse.y = (event.motion.y / float(Graphics::screen_y) *-2.0f + 1.0f);
		}
	
		Vector2f pos = make_vector(0.0f, 0.0f); //pos == mouse position.
		if (current) {
			Vector2f size = current->size();
			Box2f viewport;
			viewport.min = -0.5f * size;
			viewport.max =  0.5f * size;
			Box2f screen;
			screen.min = make_vector(-Graphics::aspectf(), -1.0f);
			screen.max = make_vector(Graphics::aspectf(),  1.0f);
			pos = current->get_mouse(viewport, screen, mouse);
		}
	
	
		if (tween == TWEEN_NONE && !snap_tweening && !edit_mode && !edit_point && !edit_scale && !edit_string && !scrolling && !zooming && !drawing && !boxing && current && current->tween_target && dynamic_cast< Module * >(current->tween_target)) {
			Module *mod = dynamic_cast< Module * >(current->tween_target);
			Vector2f size = current->size();
			Box2f viewport;
			viewport.min = -0.5f * size;
			viewport.max =  0.5f * size;
			Box2f screen;
			screen.min = make_vector(-Graphics::aspectf(), -1.0f);
			screen.max = make_vector(Graphics::aspectf(),  1.0f);
			Vector2f local = current->get_mouse(viewport, screen, mouse);
			local -= mod->position;
			local /= mod->scale;
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				assert(SDL_BUTTON(event.button.button) < 256);
				assert(!(mouse_locked & SDL_BUTTON(event.button.button)));
				mouse_locked |= SDL_BUTTON(event.button.button);
			}
			if (event.type == SDL_MOUSEBUTTONUP) {
				assert(SDL_BUTTON(event.button.button) < 256);
				mouse_locked &= ~SDL_BUTTON(event.button.button);
			}
			if (mod->handle_event(event, local)) {
				return;
			}
			if (mouse_locked) return;
		}
		assert(!mouse_locked);
	
		if (event.type == SDL_MOUSEMOTION && (tween == TWEEN_NONE)) {
			if (scrolling) {
				mouse_vel.x += event.motion.xrel / float(Graphics::screen_x) * 2.0f * Graphics::aspectf();
				mouse_vel.y += event.motion.yrel / float(Graphics::screen_y) * -2.0f;
			}
			if (zooming) {
				mouse_vel.z += event.motion.yrel / float(Graphics::screen_y) * -2.0f;
			}
			if (!edit_mode && current && drawing && !current->scribbles.empty()) {
				if (current->scribbles.back().empty() || length(current->scribbles.back().back() - pos) > 0.005 * current->screen_height) {
					current->scribbles.back().push_back(pos);
				}
			}
			if (current && boxing) {
				current->box.min = min(box_corner, pos);
				current->box.max = max(box_corner, pos);
			}
		}
		if (!edit_mode && current && !drawing && !zooming && !scrolling && !edit_string && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_b) {
			if (current->box_fade > 0.0f) {
				current->show_box = false;
				boxing = false;
			} else {
				box_corner = pos;
				boxing = true;
				current->box.min = current->box.max = box_corner;
				current->show_box = true;
			}
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			if (boxing) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					boxing = false;
				} else if (event.button.button == SDL_BUTTON_RIGHT) {
					current->show_box = false;
					boxing = false;
				}
				return;
			}
			if (event.button.button == SDL_BUTTON_LEFT) {
				mouse_vel.xy = make_vector(0.0f, 0.0f);
				scrolling = true;
				fix_target = (unsigned)-1;
				show = SHOW_CURRENT;
			} else if (!edit_mode && event.button.button == SDL_BUTTON_MIDDLE && current) {
				drawing = true;
				current->scribbles.push_back(vector< Vector2f >());
			} else if (event.button.button == SDL_BUTTON_RIGHT) {
				mouse_vel.z = 0.0f;
				zooming = true;
				fix_target = (unsigned)-1;
				show = SHOW_CURRENT;
			}
			if (event.button.button == SDL_BUTTON_MIDDLE && current && edit_mode) {
				source_module = NULL;
				float dis = 0;
	
				for (auto m = current->modules.begin(); m != current->modules.end(); ++m) {
					for (unsigned int i = 0; i < (*m)->out_ports.size(); ++i) {
						Vector2f p = (*m)->out_ports[i]->position * (*m)->scale + (*m)->position;
						float test = length_squared(pos - p);
						if (test < dis || source_module == NULL) {
							cout << "Setting source module to " << *m << "." << endl; //DEBUG
							source_module = *m;
							source_port = i;
							dis = test;
						}
					}
				}
				if (dis > 0.05f * 0.05f * current->screen_height * current->screen_height) {
					source_module = NULL;
				}
				if (source_module) {
					cout << "Started connection from " << source_module << "." << endl;
				}
			}
			if (event.button.button != SDL_BUTTON_MIDDLE && ((SDL_GetModState() & KMOD_SHIFT) || edit_mode) && current) {
				if (current->tween_target) {
					edit_point = &(current->tween_target->position);
					edit_scale = &(current->tween_target->scale);
				}
				//check for area or preview:
				if (length(pos - current->area.min) < 0.01f * current->screen_height) {
					cout << "area min" << endl;
					edit_point = &(current->area.min);
				}
				if (length(pos - current->area.max) < 0.01f * current->screen_height) {
					cout << "area max" << endl;
					edit_point = &(current->area.max);
				}
				for (auto p = current->previews.begin(); p != current->previews.end(); ++p) {
					if (length(pos - p->min) < 0.01f * current->screen_height) {
						edit_point = &(p->min);
					}
					if (length(pos - p->max) < 0.01f * current->screen_height) {
						edit_point = &(p->max);
					}
				}
				for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
					if (length(pos - *p) < 0.01f * current->screen_height) {
						edit_point = &(*p);
					}
				}
	
				if (edit_mode && (SDL_GetModState() & KMOD_SHIFT)) {
					edit_point = NULL;
					edit_scale = NULL;
					CloseSeg close;
					get_close_seg(pos, close);
					if (event.button.button == SDL_BUTTON_LEFT) {
						if (close.point_con && close.point_dis < 0.01f * current->screen_height) {
							if (close.point == 0) {
								edit_point = &(close.point_con->start);
								close.point_con->have_start = true;
							} else if (close.point - 1 < close.point_con->points.size()) {
								edit_point = &(close.point_con->points[close.point - 1]);
							} else {
								edit_point = &(close.point_con->end);
								close.point_con->have_end = true;
							}
						}
					} else { //right button.
						zooming = false;
						scrolling = true;
						if (close.seg_con && close.seg_dis < 0.01f * current->screen_height) {
							if (close.seg > close.seg_con->points.size()) {
								close.seg = close.seg_con->points.size(); //just be sure!
							}
							close.seg_con->points.insert(close.seg_con->points.begin() + close.seg, close.seg_at);
							edit_point = &(close.seg_con->points[close.seg]);
						}
					}
				}
			}
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			scrolling = zooming = false;
			if (event.button.button == SDL_BUTTON_MIDDLE) {
				drawing = false;
				if (source_module && current) {
					
					bool found = false;
					Module *dest_module = NULL;
					unsigned int dest_port = 0;
					float dis = 0.0f;
					PortData *source_data = source_module->out_ports.at(source_port);
					for (auto m = current->modules.begin(); m != current->modules.end(); ++m) {
						if (*m == source_module) found = true;
						for (unsigned int i = 0; i < (*m)->in_ports.size(); ++i) {
							Vector2f p = (*m)->in_ports[i]->position * (*m)->scale + (*m)->position;
							if (!(*m)->in_ports[i]->compatible(source_data)) continue; //only allow compatible connections.
							float test = length_squared(pos - p);
							if (test < dis || dest_module == NULL) {
								dest_module = *m;
								dest_port = i;
								dis = test;
							}
						}
					}
					if (dis > 0.05f * 0.05f * current->screen_height * current->screen_height) {
						dest_module = NULL;
					}
					if (found && source_module && dest_module) {
						cout << "Adding connection." << endl;
						current->connections.push_back(Connection());
						current->connections.back().from_module = source_module;
						current->connections.back().to_module = dest_module;
						current->connections.back().from_port = source_port;
						current->connections.back().to_port = dest_port;
					} else {
						cout << "Canceled connection." << endl;
					}
				}
				cout << "Connection from " << source_module << " is over." << endl;
				source_module = NULL;
			}
			if (edit_point) mouse_vel.xy = make_vector(0.0f, 0.0f);
			if (edit_scale) mouse_vel.z = 0.0f;
			edit_point = NULL;
			edit_scale = NULL;
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
			edit_mode = !edit_mode;
		}
		if (current && !edit_string) {
			//fixes:
			Box2f screen_box;
			screen_box.min = current->screen_center - current->screen_height * make_vector(Graphics::aspectf(), 1.0f);
			screen_box.max = current->screen_center + current->screen_height * make_vector(Graphics::aspectf(), 1.0f);
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
				show = SHOW_CURRENT;
				Vector2f ref = current->screen_center;
				if (event.key.keysym.mod & KMOD_SHIFT) {
					ref = mouse * current->screen_height / 2.0f + current->screen_center;
				}
				fix_target = (unsigned)-1;
				float dis = numeric_limits< float >::infinity();
				for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
					if (!screen_box.contains(*p)) continue;
					float test = length_squared(*p - ref);
					if (test < dis) {
						dis = test;
						fix_target = p - current->fixes.begin();
						if (event.key.keysym.mod & KMOD_SHIFT) {
							show = SHOW_DEFAULT;
						}
					}
				}
			}
			if (event.type == SDL_KEYDOWN && (
					event.key.keysym.sym == SDLK_RIGHT
					|| event.key.keysym.sym == SDLK_LEFT
					|| event.key.keysym.sym == SDLK_UP
					|| event.key.keysym.sym == SDLK_DOWN)) {
				Vector2f dir = make_vector(0.0f, 0.0f);
				if (event.key.keysym.sym == SDLK_UP) {
					dir = make_vector( 0.0f, 1.0f);
				} else if (event.key.keysym.sym == SDLK_DOWN) {
					dir = make_vector( 0.0f,-1.0f);
				} else if (event.key.keysym.sym == SDLK_RIGHT) {
					dir = make_vector( 1.0f, 0.0f);
				} else if (event.key.keysym.sym == SDLK_LEFT) {
					dir = make_vector(-1.0f, 0.0f);
				} else {
					assert(0);
				}
				float dis = numeric_limits< float >::infinity();
				unsigned int old_fix = fix_target;
				Vector2f c = current->screen_center;
				for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
					Vector2f to = *p - c;
					float len = length(to);
					if (len > 3.0f * current->screen_height) continue;
					if (len <= 0.0f * current->screen_height) continue;
					if ((to / len) * dir < 0.5f) continue;
					float test = length(to + perpendicular(dir) * 3.0f * (to * perpendicular(dir))) ;
					if (test < dis && (p - current->fixes.begin() != old_fix)) {
						dis = test;
						fix_target = p - current->fixes.begin();
					}
				}
			}
		}
		
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
			Item *item = current->tween_target;
			if (edit_mode) {
				static float stored_res = 0.0f;
				if (!item) {
					stored_res = 0.0f;
				} else {
					Vector2ui pix = make_vector(0U, 0U);
					if (!item->get_pixel_size(pix)) {
						stored_res = 0.0f;
					} else {
						if (event.key.keysym.mod & KMOD_SHIFT) {
							stored_res = item->size().y * item->scale / pix.y;
						} else if (stored_res > 0.0f) {
							item->scale = (pix.y / item->size().y) * stored_res;
						}
					}
				}
			}
		}

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
			save("dump.hack");
		}

		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2) {
			char name[1000];
			{
				unsigned int ind = 0;
				do {
					snprintf(name, 1000, "slide%03d.png",ind);
					++ind;
				} while (exists(name));
			}
			LOG_INFO("Saving screen to " << name << ".");
			draw();
			vector< uint32_t > data(Graphics::screen_x * Graphics::screen_y, 0);
			glReadPixels(0, 0, Graphics::screen_x, Graphics::screen_y, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
			save_png(name, Graphics::screen_x, Graphics::screen_y, &data[0]);

		}
	
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F5) {
			ofstream data("paste_data");
			copy(data);
			cout << "Copied." << endl;
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F6) {
			ifstream data("paste_data");
			paste(data);
			cout << "Pasted." << endl;
		}
		if (!edit_string) {
			if (event.type == SDL_KEYDOWN && edit_mode && !edit_point && current && event.key.keysym.sym == SDLK_e) {
				//expand frame area to cover screen.
				Box2f screen_box;
				screen_box.min = current->screen_center - current->screen_height * make_vector(Graphics::aspectf(), 1.0f) * 0.5f;
				screen_box.max = current->screen_center + current->screen_height * make_vector(Graphics::aspectf(), 1.0f) * 0.5f;
				current->area.min = min(current->area.min, screen_box.min);
				current->area.max = max(current->area.max, screen_box.max);
			}
			if (event.type == SDL_KEYDOWN && edit_mode && !edit_point && current && event.key.keysym.sym == SDLK_f && !(event.key.keysym.mod & KMOD_SHIFT)) {
				current->fixes.push_back(current->screen_center);
			}
			if (event.type == SDL_KEYDOWN && edit_mode && edit_point && current && event.key.keysym.sym == SDLK_f && (event.key.keysym.mod & KMOD_SHIFT)) {
				for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
					if (&(*p) == edit_point) {
						current->fixes.erase(p);
						edit_point = NULL;
						break;
					}
				}
			}
			if (event.type == SDL_KEYDOWN && edit_mode && !edit_point && current && event.key.keysym.sym == SDLK_p && !(event.key.keysym.mod & KMOD_SHIFT)) {
				Box2f internal, trash;
				current->get_boxes(internal, trash);
				current->previews.push_back(internal);
			}
			if (event.type == SDL_KEYDOWN && edit_mode && edit_point && current && current->previews.size() > 1 && event.key.keysym.sym == SDLK_p && (event.key.keysym.mod & KMOD_SHIFT)) {
				for (auto p = current->previews.begin(); p != current->previews.end(); ++p) {
					if (edit_point == &(p->min) || edit_point == &(p->max)) {
						current->previews.erase(p);
						edit_point = NULL;
						break;
					}
				}
			}
		} //edit_string
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F3) {
			set< Frame * > in_stack;
			mouse_vel = make_vector(0.0f, 0.0f, 0.0f);
			for (unsigned int s = 0; s < stack.size(); ++s) {
				in_stack.insert(stack[s]);
			}
			for (auto i = items.begin(); i != items.end(); ++i) {
				Frame *f = dynamic_cast< Frame * >(i->second);
				if (!f) continue;
				if (in_stack.count(f)) continue;
				f->screen_height = 2.0f;
				assert(f->previews.size());
				if (!f->previews.empty()) {
					f->preview = f->previews[0];
				}
				f->screen_center = f->preview.center();
				f->correct_camera();
			}
		}
		if (!edit_string) {
			if (!snap_tweening) {
			/*	snap tweening was a mistake
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s) {
					if (target_snap > snaps.size()) {
						target_snap = snaps.size();
					}
					snaps.insert(snaps.begin() + target_snap, Snap());
					store_snap(snaps[target_snap]);
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c && target_snap < snaps.size()) {
					Snap &target = snaps[target_snap];
					if (!target.screens.count(current)) {
						target.screens.insert(make_pair(current, Screen()));
					}
					target.screens[current].height = current->screen_height;
					target.screens[current].center = current->screen_center;
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_UP) {
					--target_snap;
					if (target_snap >= snaps.size()) {
							target_snap = 0;
					}
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DOWN) {
					++target_snap;
					if (target_snap >= snaps.size()) {
							target_snap = snaps.size();
					}
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
					--target_snap;
					if (target_snap >= snaps.size()) {
							target_snap = 0;
					}
					if (target_snap < snaps.size()) {
						store_snap(previous_snap, snaps[target_snap]);
						snap_tween = 0.0f;
						snap_tweening = true;
					}
				}
				if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT) {
					++target_snap;
					if (target_snap >= snaps.size()) {
							target_snap = snaps.size() - 1;
					}
					if (target_snap < snaps.size()) {
						store_snap(previous_snap, snaps[target_snap]);
						snap_tween = 0.0f;
						snap_tweening = true;
					}
				}
				*/
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_x && edit_mode) {
				if (current && (SDL_GetModState() && KMOD_SHIFT) && edit_mode) {
					//grab shift-x to delete points:
					CloseSeg close;
					get_close_seg(pos, close);
					if (close.point_con && close.point_dis < 0.01f * current->screen_height) {
						if (close.point == 0) {
							close.point_con->have_start = false;
						} else if (close.point - 1 < close.point_con->points.size()) {
							close.point_con->points.erase(close.point_con->points.begin() + (close.point - 1));
						} else {
							close.point_con->have_end = false;
						}
					} else if (close.seg_con && close.seg_dis < 0.01f * current->screen_height) {
						//delete the whole connection.
						for (auto c = current->connections.begin(); c != current->connections.end(); ++c) {
							if (close.seg_con == &(*c)) {
								current->connections.erase(c);
								break;
							}
						}
					}
				} else if (current && current->tween_target) {
					if (items.count(current->tween_target->id)) {
						items.erase(items.find(current->tween_target->id));
					}
					bool again = true;
					while (again) {
						again = false;
						for (auto i = current->items.begin(); i != current->items.end(); ++i) {
							if (*i == current->tween_target) {
								current->items.erase(i);
								again = true;
								break;
							}
						}
					}
					Module * current_module = dynamic_cast< Module * >(current->tween_target);
					if (current_module) {
						again = true;
						while (again) {
							again = false;
							for (auto i = current->modules.begin(); i != current->modules.end(); ++i) {
								if (*i == current_module) {
									current->modules.erase(i);
									again = true;
									break;
								}
							}
						}
						//erase all connections to the deleted:
						for (unsigned int c = 0; c < current->connections.size();) {
							if (current->connections[c].from_module == current_module || current->connections[c].to_module == current_module) {
								current->connections.erase(current->connections.begin() + c);
							} else {
								++c;
							}
						}
					}
					delete current->tween_target; //Deletify!
					current->tween_target = NULL;
				}
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_a) {
				if (event.key.keysym.mod & KMOD_SHIFT && tween == TWEEN_NONE) {
					show = SHOW_DEFAULT;
				} else {
					tween = TWEEN_IN;
					fix_target = (unsigned)-1;
					show = SHOW_CURRENT;
				}
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z) {
				if (event.key.keysym.mod & KMOD_SHIFT && tween == TWEEN_NONE) {
					show = SHOW_ALL;
				} else {
					if (current && current->tween == 0.0f && !stack.empty()) {
						stack.back()->tween_target = current;
						stack.back()->tween = 1.0f;
						current = stack.back();
						stack.pop_back();
					}
					tween = TWEEN_OUT;
					fix_target = (unsigned)-1;
					show = SHOW_CURRENT;
				}
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
				if (current && !drawing && !current->scribbles.empty()) {
					if (event.key.keysym.mod & KMOD_SHIFT) {
						current->scribbles.clear();
					} else {
						current->scribbles.pop_back();
					}
				}
			}
		} else { //if (!edit_string)
			if (edit_string_cursor > edit_string->size()) {
				edit_string_cursor = edit_string->size();
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_HOME) {
				edit_string_cursor = 0;
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_END) {
				edit_string_cursor = edit_string->size();
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_BACKSPACE) {
				if (edit_string->size() && edit_string_cursor) {
					edit_string->erase(edit_string_cursor - 1, 1);
					--edit_string_cursor;
				}
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DELETE) {
				if (edit_string_cursor < edit_string->size()) {
					edit_string->erase(edit_string_cursor, 1);
				}
			}
			//intercept 'CTRL-d':
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_d && (event.key.keysym.mod & KMOD_CTRL)) {
				if (current && current->tween_target) {
					string in = get_string(current->tween_target);
					edit_string->insert(edit_string_cursor, in);
					edit_string_cursor += in.size();
				}
			} else if (event.type == SDL_KEYDOWN && isprint(event.key.keysym.unicode)) {
				string in = "";
				in = (char)event.key.keysym.unicode;
				edit_string->insert(edit_string_cursor, in);
				edit_string_cursor += in.size();
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
				if (edit_string_cursor) --edit_string_cursor;
			}
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT) {
				if (edit_string_cursor < edit_string->size()) ++edit_string_cursor;
			}
		}
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
			if (edit_string) {
				if (edit_string == &create_string) {
					if (create_string.size()) {
						Item *created = NULL;
						if (create_string[0] == ':') {
							create_string.erase(0,1);
							Image *image = new Image;
							image->path = create_string;
							created = image;
							created->scale = 0.25f;
						} else if (create_string[0] == ';') {
							create_string.erase(0,1);
							Equation *equation = new Equation(create_string);
							created = equation;
							created->scale = 1.0f;
						} else if (create_string[0] == '`' && current && dynamic_cast< String * >(current->tween_target)) {
							Equation *equation = new Equation(dynamic_cast< String * >(current->tween_target)->text);
							created = equation;
							created->scale = 1.0f;
						} else if (create_string[0] == '+') {
							create_string.erase(0,1);
							Module *module = create_module(create_string);
							created = module;
							if (created) {
								created->scale = 0.25f;
							}
						} else if (create_string[0] == '~') {
							create_string.erase(0,1);
							Frame *frame = new Frame;
							created = frame;
							created->scale = 0.5f;
						} else {
							String *str = new String;
							str->text = create_string;
							created = str;
							created->scale = 0.1f;
						}
						if (created) {
							if (current) {
								Vector2f size = current->size();
								Box2f viewport;
								viewport.min = -0.5f * size;
								viewport.max =  0.5f * size;
								Box2f screen;
								screen.min = make_vector(-Graphics::aspectf(), -1.0f);
								screen.max = make_vector(Graphics::aspectf(),  1.0f);
								created->position = current->get_mouse(viewport, screen, mouse);
								current->items.push_back(created);
								if (dynamic_cast< Module * >(created)) {
									current->modules.push_back(dynamic_cast< Module * >(created));
								}
							} else if (dynamic_cast< Frame * >(created)) {
								current = dynamic_cast< Frame * >(created);
								current->tween_in = 1.0f;
							} else {
								delete created;
								created = NULL;
								cout << "Whoops, nothing to put that in!" << endl;
							}
							if (created) {
								items.insert(make_pair(created->id, created));
							}
						} else {
							cout << "Well, that creation string failed. Sorry." << endl;
						}
					}
				}
				edit_string = NULL;
			} else if (event.key.keysym.mod & KMOD_CTRL) {
				create_string = "";
				edit_string = &create_string;
				edit_string_cursor = 0;
			} else if (current && dynamic_cast< String * >(current->tween_target)) {
				edit_string = &(dynamic_cast< String * >(current->tween_target)->text);
				edit_string_cursor = edit_string->size();
			} else if (current && dynamic_cast< Image * >(current->tween_target)) {
				edit_string = &(dynamic_cast< Image * >(current->tween_target)->path);
				edit_string_cursor = edit_string->size();
			} else if (current && dynamic_cast< Equation * >(current->tween_target)) {
				edit_string = &(dynamic_cast< Equation * >(current->tween_target)->equation);
				edit_string_cursor = edit_string->size();
			}
		}
	}
	
	//helper that sets the tween_target and tween values properly on a stack:
	void tidy_stack(vector< Frame * > &stack, Frame *&current, float last_tween = 1.0f) {
		for (unsigned int s = 0; s < stack.size(); ++s) {
			stack[s]->tween_in = 1.0f;
			if (s + 1 < stack.size()) {
				stack[s]->tween_target = stack[s];
				stack[s]->tween = 1.0f;
			} else {
				stack[s]->tween_target = current;
				stack[s]->tween = last_tween;
			}
		}
		current->tween_in = last_tween;
		current->tween = 0.0f;
		current->tween_target = NULL;
		if (last_tween != 1.0f && !stack.empty()) {
			current = stack.back();
			stack.pop_back();
		}
	}
	
	void PresHack::update(float elapsed_time) {
		if (!current) return;
		if (snap_tweening) {
			Snap const &next_snap = snaps[target_snap];
			snap_tween += elapsed_time;
			if (snap_tween >= 1.0f) {
				stack = next_snap.stack;
				current = next_snap.current;
				tidy_stack(stack, current);
				for (auto s = next_snap.screens.begin(); s != next_snap.screens.end(); ++s) {
					s->first->screen_height = s->second.height;
					s->first->screen_center = s->second.center;
				}
				snap_tweening = false;
			} else {
				unsigned int common = 0;
	
				{ //find common:
					vector< Frame * > a = previous_snap.stack;
					a.push_back(previous_snap.current);
					vector< Frame * > b = next_snap.stack;
					b.push_back(next_snap.current);
					while (common < a.size() && common < b.size() && a[common] == b[common]) {
						++common;
					}
				}
				unsigned int moves = previous_snap.stack.size() + next_snap.stack.size() + 2 - 2*common;
				unsigned int pos = int(moves * snap_tween);
				float pos_frac = moves * snap_tween - pos;
				if (pos < previous_snap.stack.size() + 1 - common) {
					stack = previous_snap.stack;
					stack.push_back(previous_snap.current);
					assert(pos <= stack.size());
					stack.erase(stack.end()-pos, stack.end());
					if (!stack.empty()) {
						current = stack.back();
						stack.pop_back();
						tidy_stack(stack, current, 1.0f - pos_frac);
					} else {
						current = NULL;
						cout << "    Empty stack!" << endl; //DEBUG-ish.
					}
				} else if (pos - (previous_snap.stack.size() + 1 - common) < next_snap.stack.size() + 1 - common) {
					stack = next_snap.stack;
					stack.push_back(next_snap.current);
					pos -= previous_snap.stack.size() + 1 - common;
					assert(pos + common + 1 <= stack.size());
					stack.erase(stack.begin() + common + pos + 1, stack.end());
					if (!stack.empty()) {
						current = stack.back();
						stack.pop_back();
						tidy_stack(stack, current, pos_frac);
					} else {
						current = NULL;
						cout << "    Empty stack!" << endl; //DEBUG-ish.
					}
				} else {
					if (pos != 0) {
						cout << "Invalid position: " << pos << endl;
					}
				}
				//cameras:
				auto n = next_snap.screens.begin();
				auto p = previous_snap.screens.begin();
				while (n != next_snap.screens.end() && p != previous_snap.screens.end()) {
					if (p->first != n->first) {
						cout << "Error: expecting corresponding screens from previous to next." << endl;
						if (p->first < n->first) ++p;
						else ++n;
						continue;
					}
					p->first->screen_height = p->second.height + snap_tween * (n->second.height - p->second.height);
					p->first->screen_center = p->second.center + snap_tween * (n->second.center - p->second.center);
					++p; ++n;
				}
			}
		}
		if (current->show_box) {
			current->box_fade += elapsed_time;
			if (current->box_fade > 1.0f) current->box_fade = 1.0f;
		} else {
			current->box_fade -= elapsed_time * 4.0f;
			if (current->box_fade < 0.0f) current->box_fade = 0.0f;
		}
		if (current->tween_target == NULL) {
			tween = TWEEN_NONE;
			current->tween = 0.0f;
		}
		if (tween == TWEEN_IN) {
			current->tween += elapsed_time * current->tween_speed();
			if (current->tween > 1.0f) current->tween = 1.0f;
			Frame *next = dynamic_cast< Frame * >(current->tween_target);
			if (next) {
				next->tween_in += elapsed_time * next->tween_in_speed();
				if (next->tween_in >= 1.0f) {
					next->tween_in = 1.0f;
				}
				if (next->tween_in == 1.0f && current->tween == 1.0f) {
					stack.push_back(current);
					current = next;
					current->tween = 0.0f;
					tween = TWEEN_NONE;
				}
			}
		}
		if (tween == TWEEN_OUT) {
			current->tween -= elapsed_time * current->tween_speed();
			if (current->tween < 0.0f) current->tween = 0.0f;
			Frame *last = dynamic_cast< Frame * >(current->tween_target);
			if (last) {
				last->tween_in -= elapsed_time * last->tween_in_speed();
				if (last->tween_in < 0.0f) last->tween_in = 0.0f;
			}
			if (current->tween == 0.0f && (!last || last->tween_in == 0.0f)) {
				tween = TWEEN_NONE;
			}
		}
		if (tween == TWEEN_NONE) {
			if (edit_point && scrolling) {
				*edit_point += mouse_vel.xy * current->screen_height * elapsed_time;
			} else {
				current->screen_center -= mouse_vel.xy * current->screen_height * elapsed_time;
				if (fix_target < current->fixes.size()) {
					mouse_vel.xy = powf(0.5f, elapsed_time / 0.5f) * mouse_vel.xy;
					Vector2f targ = current->fixes[fix_target];
					Vector2f ofs = targ - current->screen_center;
					float len = length(ofs);
					float step = elapsed_time * current->screen_height * 0.01f + (1.0f - powf(0.5f, elapsed_time / 0.10f)) * len;
					if (step > len) {
						step = len;
					}
					if (len != 0.0f) {
						current->screen_center += ofs * (step / len);
					}
					//current->screen_center = current->fixes[fix_target] + powf(0.5f, elapsed_time / 0.15f) * (current->screen_center - current->fixes[fix_target]);
				}
				//update current preview:
				float dis = numeric_limits< float >::infinity();
				for (auto p = current->previews.begin(); p != current->previews.end(); ++p) {
					float test = length_squared(p->center() - current->screen_center);
					if (test < dis) {
						dis = test;
						current->preview = *p;
					}
				}
			}
			if (edit_scale && zooming) {
				*edit_scale *= powf(2.0f, mouse_vel.z * elapsed_time);
			} else {
				current->screen_height *= powf(2.0f, mouse_vel.z * elapsed_time);
				if (show != SHOW_CURRENT) {
					float targ = 2.0f;
					if (show == SHOW_ALL) {
						targ = current->area.size().y;
					}
					mouse_vel.z = powf(0.5f, elapsed_time / 0.5f) * mouse_vel.z;
					Vector2f old_local;
					{
						old_local = mouse * current->screen_height / 2.0f + current->screen_center;
					}
					current->screen_height += (targ - current->screen_height) * (1.0f - powf(0.5f, elapsed_time / 0.2f));
					Vector2f new_local;
					{
						new_local = mouse * current->screen_height / 2.0f + current->screen_center;
					}
					current->screen_center += old_local - new_local;
				}
			}
			if (!edit_mode) {
				current->correct_camera();
			}
			Vector2f size = current->size();
			if (size.y * Graphics::aspectf() > size.x) {
				size.x = size.y * Graphics::aspectf();
			} else {
				size.y = size.x / Graphics::aspectf();
			}
			Box2f viewport;
			viewport.min = -0.5f * size;
			viewport.max =  0.5f * size;
			Box2f screen;
			screen.min = make_vector(-Graphics::aspectf(), -1.0f);
			screen.max = make_vector(Graphics::aspectf(),  1.0f);
			if (!edit_point && !edit_scale && !scrolling && !zooming && !snap_tweening) {
				if (!mouse_locked) {
					current->set_target(viewport, screen, mouse);
				}
			}
		}
		if (current) {
			current->update_modules(elapsed_time);
			if (dynamic_cast< Frame * >(current->tween_target) && dynamic_cast< Frame * >(current->tween_target) != current) {
				dynamic_cast< Frame * >(current->tween_target)->update_modules(elapsed_time);
			}
		}
		mouse_vel *= powf(10.0f, -elapsed_time);
	}
	
	void PresHack::draw() {
		start_hq_frame();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glScalef(1.0f / Graphics::aspectf(), 1.0f, 1.0f);
		Vector2f size = make_vector(1.0f, 1.0f);
		if (current) {
			size = current->size();
		}
		if (size.y * Graphics::aspectf() > size.x) {
			size.x = size.y * Graphics::aspectf();
		} else {
			size.y = size.x / Graphics::aspectf();
		}
		float scale = Graphics::screen_y / size.y;
		Box2f viewport;
		viewport.min = -0.5f * size;
		viewport.max =  0.5f * size;
		Box2f screen;
		screen.min = make_vector(-Graphics::aspectf(), -1.0f);
		screen.max = make_vector(Graphics::aspectf(),  1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	
		if (current) {
			current->draw(viewport, screen, scale, 3);
	
			//set up viewport to draw in "current":
	
			//Lots of set-up for this one:
			Box2f internal_box, external_box;
			current->get_boxes(internal_box, external_box);
	
	
			{ //transform the viewport and scale info:
				float scale_x = (external_box.max.x-external_box.min.x) / (internal_box.max.x-internal_box.min.x);
				float scale_y = (external_box.max.y-external_box.min.y) / (internal_box.max.y-internal_box.min.y);
				if (scale_x > scale_y) scale *= scale_x;
				else scale *= scale_y;
	
				viewport.min -= external_box.min;
				viewport.max -= external_box.min;
				viewport.min.x /= scale_x;
				viewport.min.y /= scale_y;
				viewport.max.x /= scale_x;
				viewport.max.y /= scale_y;
				viewport.min += internal_box.min;
				viewport.max += internal_box.min;
			}
	
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glBoxToBox(viewport, screen);
	
			if (edit_mode) {
				Vector2f pos;
				{
					Vector2f size = current->size();
					Box2f viewport;
					viewport.min = -0.5f * size;
					viewport.max =  0.5f * size;
					Box2f screen;
					screen.min = make_vector(-Graphics::aspectf(), -1.0f);
					screen.max = make_vector(Graphics::aspectf(),  1.0f);
					pos = current->get_mouse(viewport, screen, mouse);
				}
		
			for (auto p = current->previews.begin(); p != current->previews.end(); ++p) {
				glBegin(GL_LINE_LOOP);
				glColor3f(0.7f, 0.2f, 0.2f);
				glVertex2f(p->min.x, p->min.y);
				glVertex2f(p->min.x, p->max.y);
				glVertex2f(p->max.x, p->max.y);
				glVertex2f(p->max.x, p->min.y);
				glEnd();
			}
			
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(1, 0xf0);
			glBegin(GL_LINE_LOOP);
			glColor3f(1.0f, 0.8f, 0.0f);
			glVertex2f(current->preview.min.x, current->preview.min.y);
			glVertex2f(current->preview.min.x, current->preview.max.y);
			glVertex2f(current->preview.max.x, current->preview.max.y);
			glVertex2f(current->preview.max.x, current->preview.min.y);
			glEnd();
			glDisable(GL_LINE_STIPPLE);
	
			for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
				glBegin(GL_LINES);
				if (p - current->fixes.begin() == fix_target) {
					glColor3f(0.6f, 0.8f, 0.6f);
					glVertex(*p);
					glColor3f(0.8f, 1.0f, 0.8f);
					glVertex(current->screen_center);
				} else {
					glColor3f(0.8f, 1.0f, 0.2f);
				}
				glVertex2f(p->x, p->y + 0.03f);
				glVertex2f(p->x, p->y - 0.03f);
				glVertex2f(p->x + 0.03f, p->y);
				glVertex2f(p->x - 0.03f, p->y);
				glEnd();
			}
	
	
			if (current->tween_target) {
				Vector2f center = current->tween_target->position;
				Vector2f size = current->tween_target->size() * current->tween_target->scale;
				glBegin(GL_LINE_LOOP);
				glColor3f(1.0f, 1.0f, 0.0f);
				glVertex2f(center.x + size.x * 0.5f, center.y + size.y * 0.5f);
				glVertex2f(center.x + size.x * 0.5f, center.y + size.y *-0.5f);
				glVertex2f(center.x + size.x *-0.5f, center.y + size.y *-0.5f);
				glVertex2f(center.x + size.x *-0.5f, center.y + size.y * 0.5f);
				glEnd();
	
				Module *mod = dynamic_cast< Module * >(current->tween_target);
				if (mod) {
					//draw ports
					Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
					const float LABELSIZE = 0.03f;
					float ls = LABELSIZE * current->screen_height;
					for (unsigned int i = 0; i < mod->in_ports.size(); ++i) {
						Vector2f pcenter = center + mod->scale * mod->in_ports[i]->position;
						glColor3f(1.0f, 0.0f, 1.0f);
						glBegin(GL_LINE_LOOP);
						glVertex(pcenter + current->screen_height * make_vector(0.02f,0.0f));
						glVertex(pcenter + current->screen_height * make_vector(0.0f,0.02f));
						glVertex(pcenter + current->screen_height * make_vector(-0.02f,0.0f));
						glVertex(pcenter + current->screen_height * make_vector(0.0f,-0.02f));
						glEnd();
						glColor3f(0.8f, 0.6f, 0.8f);
						gentium->draw(mod->in_ports[i]->name,
							pcenter - make_vector(
								0.025f * current->screen_height + 
								gentium->get_length(mod->in_ports[i]->name, ls),
								0.5f * ls),
							ls);
					}
					for (unsigned int i = 0; i < mod->out_ports.size(); ++i) {
						Vector2f pcenter = center + mod->scale * mod->out_ports[i]->position;
						glColor3f(1.0f, 0.0f, 1.0f);
						glBegin(GL_LINES);
						glVertex(pcenter + current->screen_height * make_vector(0.02f,0.0f));
						glVertex(pcenter + current->screen_height * make_vector(-0.02f,0.0f));
						glVertex(pcenter + current->screen_height * make_vector(0.0f,0.02f));
						glVertex(pcenter + current->screen_height * make_vector(0.0f,-0.02f));
						glEnd();
						glColor3f(0.8f, 0.6f, 0.8f);
						gentium->draw(mod->out_ports[i]->name,
							pcenter - make_vector(
								-0.025f * current->screen_height,
								0.5f * ls),
							ls);
					}
				}
			}
			if (source_module) {
				glBegin(GL_LINES);
				glColor3f(0.5f, 0.5f, 0.5f);
				glVertex(source_module->position + source_module->scale * source_module->out_ports[source_port]->position);
				glVertex(pos);
				glEnd();
			}
			if (SDL_GetModState() & KMOD_SHIFT) {
				CloseSeg close;
				get_close_seg(pos, close);
				if (close.seg_con && close.seg_dis < 0.01f * current->screen_height) {
					glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
					circle< 17 >(0.01f * current->screen_height, close.seg_at);
				}
				if (close.point_con && close.point_dis < 0.01f * current->screen_height) {
					glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
					circle< 17 >(0.01f * current->screen_height, close.point_at);
				}
			}
		} else { //belongs to "if (edit_mode)"
			if (SDL_GetModState() & KMOD_SHIFT) {
				Vector2f pos = mouse * current->screen_height / 2.0f + current->screen_center;

				unsigned int close_fix = (unsigned)-1;
				float dis = numeric_limits< float >::infinity();
				for (auto p = current->fixes.begin(); p != current->fixes.end(); ++p) {
					float test = length_squared(*p - pos);
					if (test < dis) {
						dis = test;
						close_fix = p - current->fixes.begin();
					}
				}
				if (close_fix < current->fixes.size() && show != SHOW_DEFAULT) {
					glBegin(GL_LINE_LOOP);
					float amt = (current->screen_height - 2.0f) / 2.0f;
					if (amt < 0.0f) amt = 0.0f;
					if (amt > 0.7f) amt = 0.7f;
					glColor4f(0.8f, 1.0f, 0.4f, amt);
					glVertex(current->fixes[close_fix] + make_vector(-Graphics::aspectf(),-1.0f));
					glVertex(current->fixes[close_fix] + make_vector(-Graphics::aspectf(), 1.0f));
					glVertex(current->fixes[close_fix] + make_vector( Graphics::aspectf(), 1.0f));
					glVertex(current->fixes[close_fix] + make_vector( Graphics::aspectf(),-1.0f));
					glEnd();
				}
			}

		} //end of if (edit_mode)

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}

	{
		float alpha = 1.0f;
		if (current) {
			if (current->tween_target) {
				Module *mod = dynamic_cast< Module * >(current->tween_target);
				if (mod) alpha = mod->cursor_alpha();
			}
		}
		if (edit_mode && alpha < 0.7f) {
			alpha = 0.7f;
		}
		//Mouse cursor:
		{
			float s = 0.1f;
			if (edit_mode) {
				s = 0.05f;
			}
			Vector2f along = normalize(make_vector(2.0f,-3.0f)) * s;
			Vector2f perp = perpendicular(along);
			glBegin(GL_QUADS);
			glColor4f(1.0f, 0.0f, 0.0f, alpha);
			if (edit_mode) {
				glColor4f(0.0f, 0.0f, 0.0f, alpha);
			}
			glVertex(mouse);
			glVertex(mouse);
			glVertex(mouse + along * 0.6f + perp * 0.3f);
			glVertex(mouse + along * 0.6f - perp * 0.3f);

			glVertex(mouse + along * 0.6f - perp * 0.08f);
			glVertex(mouse + along * 0.6f + perp * 0.08f);
			glVertex(mouse + along + perp * 0.15f);
			glVertex(mouse + along - perp * 0.15f);
			glEnd();
			glLineWidth(2.0f);
			glBegin(GL_LINE_LOOP);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glVertex(mouse);
			glVertex(mouse + along * 0.6f + perp * 0.3f);
			glVertex(mouse + along * 0.6f + perp * 0.08f);
			glVertex(mouse + along + perp * 0.15f);
			glVertex(mouse + along - perp * 0.15f);
			glVertex(mouse + along * 0.6f - perp * 0.08f);
			glVertex(mouse + along * 0.6f - perp * 0.3f);
			glEnd();
			glLineWidth(1.0f);
		}
	}

/*
	{ //Frame number thing:
		Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
		glColor3f(1.0f, 1.0f, 1.0f);
		ostringstream num;
		for (unsigned int i = 0; i < stack.size(); ++i) {
			num << stack[i]->id;
			num << '.';
		}
		if (current) {
			num << current->id;
		}
		gentium->draw_right(num.str(), make_vector(Graphics::aspectf(), 0.92f), 0.07f);
	}
*/

	if (edit_string) {
		Graphics::FontRef gentium = Graphics::get_font("gentium.txf");
		glColor3f(0.6f, 0.9f, 0.6f);
		float ofs = 0;
		float pos = gentium->get_length(">" + edit_string->substr(0, edit_string_cursor), 0.1f);
		if (pos > Graphics::aspectf() * 1.5) {
			ofs = Graphics::aspectf() * 1.5 - pos;
		}
		gentium->draw(">" + *edit_string , make_vector(-Graphics::aspectf() + ofs, -0.9f), 0.1f);
		glBegin(GL_LINES);
		glColor4f(0.7f, 0.9f, 0.6f, 0.7f);
		glVertex2f(-Graphics::aspectf() + ofs + pos, -0.9f);
		glVertex2f(-Graphics::aspectf() + ofs + pos, -0.8f);
		glEnd();
		if (edit_string == &create_string) {
			vector< string > long_matches;
			unsigned int match_size = 0;
			if (create_string.size() >= 1 && create_string[0] == '+') {
				//creating a module, so match module names:
				string rest = create_string.substr(1, create_string.size()-1);
				for (unsigned int j = 0; j < rest.size(); ++j) {
					if (isspace(rest[j])) {
						rest = rest.substr(0,j);
						break;
					}
				}
				//Ok, now 'rest' is the module name we've typed so far.
				map< string, ModuleCreator > const &mm = get_module_map();
				for (map< string, ModuleCreator >::const_iterator m = mm.begin(); m != mm.end(); ++m) {
					string test = m->first;
					unsigned int len = 0;
					while (len < test.size() && len < rest.size() && test[len] == rest[len]) {
						++len;
					}
					if (len > match_size) {
						match_size = len;
						long_matches.clear();
					}
					if (len == match_size) {
						string h = get_module_help(test);
						if (h != "") {
							test = test + " -- " + h;
						}
						long_matches.push_back(test);
					}
				}
			} else if (create_string.size() >= 1 && create_string[0] == ':') {
				string dir = ".";
				string file = create_string.substr(1);
				for (unsigned int i = file.size() - 1; i < file.size(); --i) {
					if (file[i] == '/') {
						dir = file.substr(0, i);
						file = file.substr(i+1);
						break;
					}
				}
				static vector< string > names;
				static string listed = "actually, no, I didn't.";
				if (dir != listed) {
					listed = dir;
					names.clear();
					list_dir(listed, names);
					sort(names.begin(), names.end());
					for (unsigned int i = 0; i < names.size(); ++i) {
						if (is_directory(listed + "/" + names[i])) {
							names[i] += '/';
						}
					}
				}
				//fill in long_matches:
				for (unsigned int i = 0; i < names.size(); ++i) {
					string test = names[i];
					unsigned int len = 0;
					while (len < test.size() && len < file.size() && test[len] == file[len]) {
						++len;
					}
					if (len > match_size) {
						match_size = len;
						long_matches.clear();
					}
					if (len == match_size) {
						long_matches.push_back(test);
					}
				}
			} else if (create_string.empty()) {
				//creating nothing just yet
				match_size = 1;
				long_matches.push_back("~ frame");
				long_matches.push_back(": image");
				long_matches.push_back("; equation");
				long_matches.push_back("+ module");
				long_matches.push_back(" (anything else is text)");
			}

			{ //draw the long_matches array:
				float pos_y = -0.88f;
				float font_height = 0.06f;
				for (unsigned int i = long_matches.size() - 1; i < long_matches.size(); --i) {
					pos_y += font_height;
					string pre = " " + long_matches[i].substr(0, match_size);
					string post = long_matches[i].substr(match_size, long_matches[i].size() - match_size);
					glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
					gentium->draw(pre + post, make_vector(-Graphics::aspectf() + 0.009f, pos_y - 0.009f), font_height);
					glColor4f(1.0f, 0.8f, 1.0f, 0.9f);
					gentium->draw(pre, make_vector(-Graphics::aspectf(), pos_y), font_height);
					glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
					gentium->draw(post, make_vector(-Graphics::aspectf() + gentium->get_length(pre, font_height), pos_y), font_height);
				}
			}
		}
	}
	if (edit_mode && target_snap < snaps.size()) {
		Snap const &target = snaps[target_snap];
		vector< Frame * > seq = target.stack;
		seq.push_back(snaps[target_snap].current);
		vector< Frame * > cur = stack;
		if (current) {
			cur.push_back(current);
		}
		reverse(cur.begin(), cur.end());
		while (!cur.empty() && seq.size() > 1 && cur.back() == seq[0]) {
			seq.erase(seq.begin());
			cur.pop_back();
		}
		seq.insert(seq.begin(),cur.begin(),cur.end());
		
		vector< Screen > screen_temps;
		for (auto s = target.screens.begin(); s != target.screens.end(); ++s) {
			screen_temps.push_back(Screen());
			screen_temps.back().height = s->first->screen_height;
			screen_temps.back().center = s->first->screen_center;
			s->first->screen_height = s->second.height;
			s->first->screen_center = s->second.center;
		}
		glBegin(GL_QUADS);
		glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
		glVertex2f(-Graphics::aspectf(),0.83f);
		glVertex2f(-Graphics::aspectf(),0.97f);
		glVertex2f(-Graphics::aspectf() + Graphics::aspectf() * (0.12f * seq.size() + 0.02f),0.97f);
		glVertex2f(-Graphics::aspectf() + Graphics::aspectf() * (0.12f * seq.size() + 0.02f),0.83f);
		glEnd();
		for (unsigned int s = 0; s < seq.size(); ++s) {
			float tween_in_temp = seq[s]->tween_in;
			seq[s]->tween_in = 1.0f;
			float tween_temp = seq[s]->tween;
			seq[s]->tween = 0.0f;

			Vector2f size = seq[s]->size();
			Box2f viewport;
			viewport.min = -0.5f * size;
			viewport.max =  0.5f * size;
			Box2f screen;
			screen.min = make_vector(-Graphics::aspectf() + s * Graphics::aspectf() * 0.12f + 0.02f * Graphics::aspectf(), 0.85f);
			screen.max = screen.min + 0.1f * make_vector(Graphics::aspectf(),  1.0f);
			seq[s]->draw(viewport, screen, scale, 2);

			seq[s]->tween_in = tween_in_temp;
			seq[s]->tween = tween_temp;
			if (s < cur.size()) {
				glBegin(GL_LINES);
				glColor3f(1.0f, 1.0f, 1.0f);
				glVertex(screen.min);
				glVertex(screen.max);
				glEnd();
			}
		}
		auto scr = screen_temps.begin();
		for (auto s = target.screens.begin(); s != target.screens.end(); ++s) {
			assert(scr != screen_temps.end());
			s->first->screen_height = scr->height;
			s->first->screen_center = scr->center;
			++scr;
		}
		assert(scr == screen_temps.end());
	}

	finish_hq_frame();

	Graphics::gl_errors("PresHack::draw");
}

void check_point(Vector2f const &pos, Connection *con, unsigned int index, Vector2f const &point, CloseSeg &close) {
	float dis = length(pos - point);
	if (close.point_con == NULL || dis < close.point_dis) {
		close.point_dis = dis;
		close.point_con = con;
		close.point_at = point;
		close.point = index;
	}
}

void check_segment(Vector2f const &pos, Connection *con, unsigned int index, Vector2f const &from, Vector2f const &to, CloseSeg &close) {
	float along = (pos - from) * (to - from);
	if (along < 0.0f) along = 0.0f;
	float len = length_squared(to - from);
	if (along > len) along = len;
	if (len != 0.0f) {
		along /= len;
	}
	Vector2f at = (to - from) * along + from;
	float dis = length(pos - at);
	if (close.seg_con == NULL || dis < close.seg_dis) {
		close.seg_dis = dis;
		close.seg_con = con;
		close.seg_along = along;
		close.seg_at = at;
		close.seg = index;
	}
}

void PresHack::get_close_seg(Vector2f const &pos, CloseSeg &close) {
	close.seg = (unsigned)-1;
	close.seg_at = make_vector( 0.0f, 0.0f);
	close.seg_along = 0.0f;
	close.seg_dis = 100000.0f;
	close.seg_con = NULL;
	close.point = (unsigned)-1;
	close.point_at = make_vector(0.0f, 0.0f);
	close.point_dis = 100000.0f;
	close.point_con = NULL;
	if (!current) return;
	for (auto c = current->connections.begin(); c != current->connections.end(); ++c) {
		Vector2f from = make_vector(0.0f, 0.0f);
		Vector2f to = make_vector(0.0f, 0.0f);
		if (c->have_start) from = c->start;
		if (c->from_module && c->from_module->out_ports.size() > c->from_port) {
			if (!c->have_start) {
				from = c->from_module->out_ports[c->from_port]->position;
			}
			from = from * c->from_module->scale + c->from_module->position;
		}
		if (c->have_end) to = c->end;
		if (c->to_module && c->to_module->in_ports.size() > c->to_port) {
			if (!c->have_end) {
				to = c->to_module->in_ports[c->to_port]->position;
			}
			to = to * c->to_module->scale + c->to_module->position;
		}

		check_point(pos, &(*c), 0, from, close);
		check_point(pos, &(*c), c->points.size() + 1, to, close);
		for (vector< Vector2f >::iterator p = c->points.begin(); p != c->points.end(); ++p) {
			check_point(pos, &(*c), p - c->points.begin() + 1, *p, close);
			check_segment(pos, &(*c), p - c->points.begin(), from, *p, close);
			from = *p;
		}
		check_segment(pos, &(*c), c->points.size(), from, to, close);
	}
}
