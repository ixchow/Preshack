#include "LL_LayeringModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/GLExtensions.hpp>
#include <Graphics/GLSLShader.hpp>
#include <Graphics/Font.hpp>
#include <Vector/Vector.hpp>
#include <Vector/VectorGL.hpp>
#include <Graphics/load_save_png.hpp>

#include <cassert>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <png.h>

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl

namespace {
	Module *create_module(const std::string &params_in) {
		string params = params_in;
		vector< string > files;
		params += ' ';
		while (params.size()) {
			if (isspace(params[0])) {
				params.erase(0,1);
				continue;
			}
			for (unsigned int c = 0; c < params.size(); ++c) {
				if (isspace(params[c])) {
					files.push_back(params.substr(0,c));
					params.erase(0,c);
					break;
				}
			}
		}
		LOG_INFO("Adding " << files.size() << " files.");
		LayeringModule *ll = new LayeringModule();
		if (files.size() == 1 && files[0].size() >= 4 && files[0].substr(files[0].size()-4,4) == ".obj") {
			ll->width = 512;
			ll->height = 512;
			ll->add_model(files[0]);
		} else {
			for (unsigned int f = 0; f < files.size(); ++f) {
				ll->add_layer(files[f]);
			}
		}
		return ll;
	}
	class Fred {
	public:
		Fred() {
			register_module("locallayering", create_module, "layer0 layer1 ...");
		}
	} fred;
}


using std::swap;

using std::ifstream;
using std::ostringstream;

REQUIRE_GL_EXTENSION(GL_ARB_window_pos);
REQUIRE_GL_EXTENSION(GL_ARB_texture_rectangle);
REQUIRE_GL_EXTENSION(GL_EXT_framebuffer_object);
REQUIRE_GL_EXTENSION(GL_ARB_depth_texture);
REQUIRE_GL_EXTENSION(GL_ARB_shader_objects);

void upload_texture(unsigned int width, unsigned int height, vector< uint32_t > &data) {
	assert(data.size() == width * height);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(data[0]));
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);
}

namespace {
	void draw_nothing(const PortData *, const Connection &, void *) {
	}
}


LayeringModule::LayeringModule() : width(0), height(0), image_tex(0), image_dirty(false), tags_tex(0), tags_dirty(true), acc_move_x(0), acc_move_y(0), dialog_point(make_vector((unsigned)-1, (unsigned)-1)), current_layer((unsigned)-1)  {
	dialog_tween = 0.0f;
	mouse_pos = make_vector(0,0);

	total_frames = 0; //e.g. "not in animation mode"
	current_frame = 0;

	camera = make_vector(1.0f, 1.0f, 1.0f);
	new_camera = camera;
	target = make_vector(0.0f, 0.0f, 0.0f);

	fixed_layers = 0;

	model_dirty = false;
	show_tags = false;
	allow_fail = false;

	no_background = true;
	out_ports.push_back(&this_port);
	this_port.name = "dialog";
	this_port.position = make_vector(0.0f, 0.0f);
	this_port() = this;
	in_ports.push_back(&anim_pos);
	anim_pos.name = "anim-pos";
	anim_pos.position = make_vector(0.0f,-0.5f);
	anim_pos.draw_fn = draw_nothing;
	anim_pos() = 0.0f;
	last_anim_pos = anim_pos();
}

LayeringModule::~LayeringModule() {
}

Vector2f LayeringModule::size() {
	if (width == 0 || height == 0) {
		return make_vector(1.0f, 1.0f);
	}
	return make_vector(1.0f * (width / float(height)), 1.0f);
}

void LayeringModule::add_layer(string filename) {
	while (layers.size() > fixed_layers) {
		layers.pop_back();
	}
	unsigned int w, h;
	layers.push_back(vector< uint32_t >());
	if (!load_png(filename, w, h, layers.back())) {
		LOG_ERROR("Error loading " << filename);
		layers.pop_back();
		return;
	}
	assert(w*h == layers.back().size());
	if (layers.size() == 1) {
		width = w;
		height = h;
	}
	if (width != w || height != h) {
		LOG_ERROR("Error loading " << filename << " -- size mis-match.");
		layers.pop_back();
		return;
	}
	{
		Vector4f sum = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
		for (unsigned int i = 0; i < layers.back().size(); ++i) {
			uint32_t col = layers.back()[i];
			Vector4f color = make_vector((col & 0xff) / 255.0f, ((col >> 8) & 0xff) / 255.0f, ((col >> 16) & 0xff) / 255.0f, ((col >> 24) & 0xff) / 255.0f);
			sum += make_vector(color.xyz * color.w, color.w);
		}
		if (sum.w) {
			sum.xyz /= sum.w;
		}
		while (colors.size() < layers.size()) {
			colors.push_back(sum.xyz);
		}
	}
	++fixed_layers;
	assert(layers.size() == fixed_layers);

	anim_filenames.push_back(vector< string >());
	anim_filenames.back().push_back(filename);
	assert(anim_filenames.size() == layers.size());

	unsigned int last_zero = filename.size()-1;
	while (last_zero < filename.size() && filename[last_zero] != '0') --last_zero;
	if (last_zero < filename.size()) {
		unsigned int first_zero = last_zero;
		while (first_zero - 1 < filename.size() && filename[first_zero - 1] == '0') --first_zero;
		assert(first_zero < filename.size());
		string pre = filename.substr(0,first_zero);
		string post = filename.substr(last_zero+1);
		LOG_INFO("Trying to load multiple frames of form " << pre << "*" << post);
		unsigned int zeros = last_zero - first_zero + 1;
		unsigned int fn = 0;
		while (1) {
			++fn;
			string num = "";
			unsigned int temp = fn;
			for (unsigned int i = 0; i < zeros; ++i) {
				num = (char)('0' + (temp % 10)) + num;
				temp /= 10;
			}
			string next_file = pre + num + post;
			unsigned int w,h;
			vector< uint32_t > data;
			bool loaded = load_png(next_file,w,h,data);
			if (!loaded) break;
			if (w != width || h != height) {
				LOG_ERROR("Frame has wrong size.");
				break;
			}
			//we actually keep track of filenames even if we don't have lots of frames:
			assert(anim_filenames.size() == layers.size());
			anim_filenames.back().push_back(next_file);
			//we don't keep track of frames if we don't have lots of frames:
			while (anim_layers.size() < layers.size()) {
				anim_layers.push_back(vector< vector< uint32_t > >());
				anim_layers.back().push_back(layers[anim_layers.size()-1]);
			}
			assert(!anim_layers.empty());
			anim_layers.back().push_back(data);
			if (total_frames < anim_layers.back().size()) {
				total_frames = anim_layers.back().size();
				LOG_INFO("Increased to " << total_frames << " frames.");
			}
		}
	}

	if (total_frames) {
		while(anim_layers.size() < layers.size()) {
			anim_layers.push_back(vector< vector< uint32_t > >());
			anim_layers.back().push_back(layers[anim_layers.size()-1]);
		}
		assert(anim_layers.size() == layers.size());
	}

	textures.push_back(0);
	glGenTextures(1, &textures.back());
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures.back());
	upload_texture(width, height, layers.back());
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	bins.push_back(DialogBin(layers.size()-1, filename));
	update_dialog();

	if (!total_frames) {
		update_regions();
	} else {
		update_anim_tags();
		current_frame = (unsigned)-1;
		set_frame(0);
	}
}

class FactCompare {
public:
	bool operator()(unsigned int a, unsigned int b) {
		assert(a < items);
		assert(b < items);
		assert(facts.size() == items * items);
		assert(tiebreak.size() == items);
		if (facts[a * items + b]) return true;
		if (facts[b * items + a]) return false;
		for (unsigned int i = 0; i < tiebreak.size(); ++i) {
			if (tiebreak[i] == b) return false;
			if (tiebreak[i] == a) return true;
		}
		assert(0);
		return false; //should never get here.
	}
	unsigned int items;
	vector< bool > facts;
	vector< unsigned int > tiebreak; //want ordering 'like tiebreak' in case of tie.
};

//used for the update_regions as well as for animation.
void extract_tags(unsigned int width, unsigned int height, unsigned int layers, vector< uint32_t > const &bits, vector< uint32_t > &tags, ListGraph &graph) {
	assert(bits.size() == width * height);
	//find regions:
	tags.resize(width*height);
	memset(&(tags[0]), '\0', sizeof(uint32_t)*width*height);
	//tag == 0 -> not visited.
	//tag == -1 -> seed.
	vector< uint32_t > seeds;
	seeds.push_back(0);
	tags[0] = (uint32_t)-1;
	const unsigned int Ofs = 4;
	static Vector2i ofs[Ofs] = {
		{{{-1, 0}}},
		{{{ 1, 0}}},
		{{{ 0,-1}}},
		{{{ 0, 1}}},
	};
	graph.lists.clear();
	graph.edges.clear();
	vector< vector< bool > > tag_adj;
	uint32_t cur_tag = 0;
	while (!seeds.empty()) {
		uint32_t seed = seeds.back();
		seeds.pop_back();
		if(tags[seed] != uint32_t(-1)) continue; //already filled this seed at some point in the past.
		++cur_tag; //get a fresh tag.
		tags[seed] = cur_tag;
		tag_adj.push_back(vector< bool >(cur_tag-1, false));
		assert(tag_adj.size() == cur_tag);
		vector< uint32_t > to_expand;
		to_expand.push_back(seed);
		uint32_t kind = bits[seed];
		graph.lists.push_back(vector< unsigned int >());
		for (unsigned int l = 0; l < layers; ++l) {
			if (kind & (1 << l)) {
				graph.lists.back().push_back(l);
			}
		}
		while (!to_expand.empty()) {
			uint32_t r = to_expand.back() / width;
			uint32_t c = to_expand.back() % width;
			to_expand.pop_back();
			for (unsigned int i = 0; i < Ofs; ++i) {
				if (uint32_t(r + ofs[i].y) >= height) continue;
				if (uint32_t(c + ofs[i].x) >= width) continue;
				uint32_t n = (r+ofs[i].y)*width+c+ofs[i].x;
				if (bits[n] == kind) { //part of this region.
					if (tags[n] == cur_tag) continue;
					tags[n] = cur_tag;
					to_expand.push_back(n);
				} else if (tags[n] != 0) {
					if (tags[n] != (unsigned)-1) {
						assert(tags[n]-1 < tag_adj.back().size());
						tag_adj.back()[tags[n]-1] = true;
					}
				} else {
					tags[n] = (uint32_t)-1; //mark as seed.
					seeds.push_back(n);
				}
			}
		}
	}
	for (unsigned int i = 0; i < tags.size(); ++i) {
		//quick check:
		assert(tags[i] != 0);
		assert(tags[i] != (unsigned)-1);
		tags[i] -= 1; //so tags are zero-based again.
	}
	//set up neighbors:
	graph.edges.resize(cur_tag);
	for (unsigned int i = 0; i < graph.edges.size(); ++i) {
		graph.edges[i].clear();
		for (unsigned int j = 0; j < tag_adj[i].size(); ++j) {
			if (tag_adj[i][j]) {
				graph.edges[j].push_back(i);
				graph.edges[i].push_back(j);
			}
		}
	}
	assert(graph.lists.size() == graph.edges.size());
}

bool LayeringModule::update_regions(bool can_fail) {
	if (width == 0 || height == 0) return true;
	if (model_dirty) render_model();
	assert(!total_frames); //should never be called in "animation mode"
	bits.resize(width*height);
	//zero:
	memset(&(bits[0]), '\0', sizeof(uint32_t)*width*height);
	//code each region:
	for (unsigned int l = 0; l < layers.size(); ++l) {
		uint32_t mask = 1 << l;
		assert(layers[l].size() == bits.size());
		for (unsigned int i = 0; i < layers[l].size(); ++i) {
			if (layers[l][i] & 0xff000000) {
				bits[i] |= mask;
			}
		}
	}
	vector< uint32_t > old_tags = tags;
	vector< vector< unsigned int > > old_lists = stacking.lists;
	//find regions:
	extract_tags(width, height, layers.size(), bits, tags, stacking);
	tags_dirty = true;

	//See about preserving old stacking info:
	if (old_tags.size() == tags.size()) {
		vector< unsigned int > votes(stacking.lists.size() * old_lists.size());
		for (unsigned int i = 0; i < tags.size(); ++i) {
			votes[tags[i] * old_lists.size() + old_tags[i]] += 1;
		}

		//build tie-breaking list:
		vector< unsigned int > tiebreak(layers.size());
		for (unsigned int i = 0; i < tiebreak.size(); ++i) {
			tiebreak[i] = i;
		}
		//current layer wants to go on top, if possible.
		if (current_layer < layers.size()) {
			tiebreak.erase(tiebreak.begin() + current_layer);
			tiebreak.push_back(current_layer);
		}

		//First build up all local rewards; i.e. how much you get in one
		//area if a fact is true:
		vector< vector< unsigned int > > local_rewards(stacking.lists.size(), vector< unsigned int >(layers.size() * layers.size(), 0));
		for (unsigned int t = 0; t < stacking.lists.size(); ++t) {
			//reward[x * layers.size() + y] --> reward x BEFORE y.
			vector< unsigned int > &reward = local_rewards[t];
			for (unsigned int to = 0; to < old_lists.size(); ++to) {
				unsigned int weight = votes[t * old_lists.size() + to];
				if (weight == 0) continue;
				vector< unsigned int > const &old_list = old_lists[to];
				for (unsigned int i = 0; i < old_list.size(); ++i) {
					for (unsigned int j = 0; j < i; ++j) {
						reward[old_list[j] * layers.size() + old_list[i]] += weight;
					}
				}
			}
		}
		//Spread local rewards to connected components:
		vector< vector< unsigned int > > global_rewards;
		{ //init with diag-as-neg1 => things never allowed to be before self.
			vector< unsigned int > blank_reward(layers.size() * layers.size(), 0);
			for (unsigned int i = 0; i < layers.size(); ++i) {
				blank_reward[i * layers.size() + i] = (unsigned)-1;
			}
			global_rewards.resize(stacking.lists.size(), blank_reward);
		}
		vector< uint32_t > masks(stacking.lists.size(), 0);
		//build masks for quick checks:
		for (unsigned int t = 0; t < stacking.lists.size(); ++t) {
			for (unsigned int i = 0; i < stacking.lists[t].size(); ++i) {
				masks[t] |= 1 << stacking.lists[t][i];
			}
		}
		LOG_INFO("Spreading reward...");
		//spread local rewards around connected components:
		for (unsigned int l1 = 0; l1 < layers.size(); ++l1) {
			for (unsigned int l2 = l1+1; l2 < layers.size(); ++l2) {
				uint32_t mask = (1 << l1) | (1 << l2);
				vector< bool > visited(stacking.lists.size(), false);
				unsigned int seed = 0;
				while (seed < stacking.lists.size()) {
					if (visited[seed]) {
						++seed;
						continue;
					}
					if (!((masks[seed] & mask) == mask)) {
						visited[seed] = true; //empty component, really.
						++seed;
						continue;
					}
					vector< unsigned int > component;
					component.push_back(seed);
					visited[seed] = true;
					unsigned int pos = 0;
					while (pos < component.size()) {
						unsigned int ind = component[pos];
						++pos;
						assert(ind < stacking.lists.size());
						assert(visited[ind]);
						assert((masks[ind] & mask) == mask);
						for (unsigned int e = 0; e < stacking.edges[ind].size(); ++e) {
							unsigned int ind2 = stacking.edges[ind][e];
							if (visited[ind2]) continue;
							if ((masks[ind2] & mask) != mask) continue;
							component.push_back(ind2);
							visited[ind2] = true;
						}
					}
					unsigned int sum_before = 0;
					unsigned int sum_after = 0;
					for (vector< unsigned int >::iterator c = component.begin(); c != component.end(); ++c) {
						assert(*c < local_rewards.size());
						sum_before += local_rewards[*c][l1 * layers.size() + l2];
						sum_after += local_rewards[*c][l2 * layers.size() + l1];
					}
					for (vector< unsigned int >::iterator c = component.begin(); c != component.end(); ++c) {
						assert(*c < global_rewards.size());
						assert(global_rewards[*c][l1 * layers.size() + l2] == 0);
						global_rewards[*c][l1 * layers.size() + l2] = sum_before;
						assert(global_rewards[*c][l2 * layers.size() + l1] == 0);
						global_rewards[*c][l2 * layers.size() + l1] = sum_after;
					}
				}
			}
		}
		LOG_INFO("Choosing facts...");
		//Ok, now choose greedily. //NB: probably could do without spread, but whatever.
		vector< vector< bool > > facts(stacking.lists.size(), vector< bool >(layers.size() * layers.size(), false));
		//DEBUG:
		/*LOG_INFO("Facts:");
		for (unsigned int t = 0; t < facts.size(); ++t) {
			for (unsigned int i = 0; i < facts[t].size(); ++i) {
				LOG_INFO("  " << i / layers.size() << " < " << i % layers.size() << " at " << t << " [" << (int)global_rewards[t][i] << "] {" << facts[t][i] << "}");
			}
		}*/
		//end DEBUG
		while (1) {
			//First we choose something that hasn't already been deduced
			// and hasn't been disallowed.
			unsigned int best_list = (unsigned)-1;
			unsigned int best_fact = (unsigned)-1;
			unsigned int best_score = 0;
			for (unsigned int t = 0; t < facts.size(); ++t) {
				for (unsigned int i = 0; i < facts[t].size(); ++i) {
					if (facts[t][i] == true) continue;
					if (global_rewards[t][i] == (unsigned)-1) continue;
					int score = global_rewards[t][i];
					if (score > 0) {
					}
					assert(score >= 0);
					if ((unsigned)score > best_score) {
						best_score = (unsigned)score;
						best_fact = i;
						best_list = t;
					}
				}
			}
			if (best_list >= stacking.lists.size()) {
				//ok, we've run out of facts to add. um. Must be done?
				break;
			}
			vector< vector< bool > > old_facts = facts;
			unsigned int a = best_fact / layers.size();
			unsigned int b = best_fact % layers.size();
			//LOG_INFO("Trying fact " << a << " < " << b << " at " << best_list << " [" << best_score << "].");
			bool ok = stacking.test_fact(layers.size(), facts, best_list, a, b);
			if (!ok) {
				facts = old_facts;
				if (can_fail) return false;
				//LOG_INFO(" ... Failed.");
			} else {
				//DEBUG: check that facts are consistent.
				stacking.verify_facts(layers.size(), facts);
			}
			global_rewards[best_list][best_fact] = (unsigned)-1; //remove from consideration.
		}
		//Now that facts have been chosen, make an order that uses 'em.
		for (unsigned int l = 0; l < stacking.lists.size(); ++l) {
			vector< unsigned int > &list = stacking.lists[l];
			FactCompare fc;
			fc.items = layers.size();
			fc.facts = facts[l];
			fc.tiebreak = tiebreak;
			sort(list.begin(), list.end(), fc);
		}
		if (!stacking.is_consistent()) {
			LOG_INFO("Oh NO! inconsistent stacking?");
			assert(0);
		}
	}

	LOG_INFO("There are " << stacking.lists.size() << " components.");
	image.resize(width*height);
	update_image();
	return true;
}

union Pix {
	uint32_t val;
	struct {
		uint8_t r,g,b,a;
	};
};

void LayeringModule::update_image() {
	assert(sizeof(Pix) == sizeof(uint32_t));
	if (image.size() != width*height) image.resize(width * height);
	assert(image.size() == tags.size());
	if (show_tags) {
		while (tag_colors.size() < stacking.lists.size()) {
			tag_colors.push_back(0xff000000 | rand());
		}
		for (unsigned int i = 0; i < tags.size(); ++i) {
			assert(tags[i] < tag_colors.size());
			image[i] = tag_colors[tags[i]];
		}
		image_dirty = true;
		return;
	}
	for (unsigned int i = 0; i < tags.size(); ++i) {
		assert(tags[i] < stacking.lists.size());
		if (stacking.lists[tags[i]].empty()) {
			image[i] = 0x00444444; //Background is gray.
			continue;
		}
		Pix pix;
		pix.val = 0x00444444;
		for (vector< unsigned int >::iterator l = stacking.lists[tags[i]].begin(); l != stacking.lists[tags[i]].end(); ++l) {
			Pix over_pix; over_pix.val = layers[*l][i];
			pix.r = (pix.r * (255 - over_pix.a) + over_pix.r * over_pix.a) / 255;
			pix.g = (pix.g * (255 - over_pix.a) + over_pix.g * over_pix.a) / 255;
			pix.b = (pix.b * (255 - over_pix.a) + over_pix.b * over_pix.a) / 255;
			pix.a = (pix.a * (255 - over_pix.a) + over_pix.a * 255) / 255;

		}

		image[i] = pix.val;
	}
	image_dirty = true;
	update_dialog();
}

GLuint LayeringModule::get_tags_tex() {
	if (tags_tex && !tags_dirty) return tags_tex;
	tags_dirty = false;
	assert(tags.size() == width * height);
	if (tags_tex == 0) {
		glGenTextures(1, &tags_tex);
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tags_tex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(tags[0]));
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	return tags_tex;
}

namespace {
Graphics::ProgramObjectRef subpixel_shader = NULL;
}

void LayeringModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	if (!subpixel_shader.ref) {
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/ll_subpixel.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		subpixel_shader = Graphics::get_program_object(frag);
		assert(subpixel_shader.ref);
		glUseProgramObjectARB(subpixel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(subpixel_shader->handle, "image"), 0);
		glUseProgramObjectARB(0);
	}

	static GLuint checker_tex = 0;
	const unsigned int CheckerSize = 64;
	if (checker_tex == 0) {
		glGenTextures(1, &checker_tex);
		vector< uint32_t > check(CheckerSize * CheckerSize);
		for (unsigned int i = 0; i < CheckerSize * CheckerSize; ++i) {
			if (((i / CheckerSize) ^ i) & 0x10) {
				check[i] = 0xff666666;
			} else {
				check[i] = 0xffcccccc;
			}
		}
		glBindTexture(GL_TEXTURE_2D, checker_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CheckerSize, CheckerSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &check[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if (width * height == 0) return;
	if (image.size() != width * height) return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	//background:
	if (!no_background) {
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(-size().x*0.5f, -size().y*0.5f);
		glVertex2f( size().x*0.5f, -size().y*0.5f);
		glVertex2f( size().x*0.5f,  size().y*0.5f);
		glVertex2f(-size().x*0.5f,  size().y*0.5f);
		glEnd();
	}

	glEnable(GL_BLEND);

	//"to widthxheight screen" sez GP_:
	glTranslatef(-0.5f * size().x, -0.5f * size().y, 0.0f);
	glScalef(size().x / width, size().y / height, 1.0f);


	if (image_dirty) {
		image_dirty = false;
		if (image_tex == 0) {
			glGenTextures(1, &image_tex);
		}
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, image_tex);
		upload_texture(width, height, image);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}

	if (!no_background) {
		glBindTexture(GL_TEXTURE_2D, checker_tex);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0,0);
		glVertex(make_vector< float >(0, 0));
		glTexCoord2f(width / float(CheckerSize),0);
		glVertex(make_vector< float >(width, 0));
		glTexCoord2f(width / float(CheckerSize),height / float(CheckerSize));
		glVertex(make_vector< float >(width, height));
		glTexCoord2f(0,height / float(CheckerSize));
		glVertex(make_vector< float >(0, height));
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	float px_size = 2.0f / Graphics::screen_y * viewport.size().x / screen_viewport.size().x * height / size().y;
	glUseProgramObjectARB(subpixel_shader->handle);
	glUniform1fARB(glGetUniformLocationARB(subpixel_shader->handle, "px_size"), px_size);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, image_tex);
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(0,0);
	glVertex(make_vector< float >(0, 0));
	glTexCoord2f(width,0);
	glVertex(make_vector< float >(width, 0));
	glTexCoord2f(width,height);
	glVertex(make_vector< float >(width, height));
	glTexCoord2f(0,height);
	glVertex(make_vector< float >(0, height));
	glEnd();
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glUseProgramObjectARB(0);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	Graphics::gl_errors("draw");
}

void LayeringModule::draw_arrow(Vector2f arrow_c1, Vector2f arrow_c2) {

	if (dialog_point.x >= width && dialog_point.y >= height) return;
	Vector2f tip;
	tip.x = (dialog_point.x / float(width) - 0.5f) * size().x * scale + position.x;
	tip.y = (dialog_point.y / float(height) - 0.5f) * size().y * scale + position.y;

	Vector2f out_c1, in_c1, out_c2, in_c2;
	const float Len = 0.2f;
	{
		Vector2f mid = (arrow_c1 + arrow_c2) * 0.5f;
		Vector2f dir = normalize(mid - tip);
		Vector2f perp = perpendicular(dir);
		const float Out = 0.17f;
		const float In  = 0.07f;
		out_c1 = tip + Len * dir + Out * perp;
		out_c2 = tip + Len * dir - Out * perp;
		in_c1 = tip + Len * dir + In * perp;
		in_c2 = tip + Len * dir - In * perp;
	}
	const float Dep = 0.1f;

	arrow_c1.y -= Dep;
	arrow_c2.y -= Dep;
	in_c1.y -= Len * Dep;
	in_c2.y -= Len * Dep;
	out_c1.y -= Len * Dep;
	out_c2.y -= Len * Dep;

	glBegin(GL_QUAD_STRIP);
	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
	glVertex(arrow_c1);
	glVertex(arrow_c2);
	glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
	glVertex(in_c1);
	glVertex(in_c2);
	glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
	glVertex(out_c1);
	glVertex(out_c2);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glVertex(tip);
	glVertex(tip);
	glEnd();

	glBegin(GL_QUAD_STRIP);
	glColor4f(0.9f, 0.9f, 0.9f, 0.6f);
	for (unsigned int i = 0; i <= 20; ++i) {
		float amt = i / 20.0f;
		Vector2f p1 = arrow_c1 + (in_c1 - arrow_c1) * amt;
		Vector2f p2 = arrow_c2 + (in_c2 - arrow_c2) * amt;
		float d = 1.0f - (1.0f - Len) * amt;
		d = Dep * cosf(M_PI * 0.5f * (1.0f - d));
		glVertex2f(p1.x, p1.y + d);
		glVertex2f(p2.x, p2.y + d);
	}
	for (unsigned int i = 0; i <= 5; ++i) {
		float amt = i / 5.0f;
		Vector2f p1 = out_c1 + (tip - out_c1) * amt;
		Vector2f p2 = out_c2 + (tip - out_c2) * amt;
		float d = Len - Len * amt;
		d = Dep * cosf(M_PI * 0.5f * (1.0f - d));
		glVertex2f(p1.x, p1.y + d);
		glVertex2f(p2.x, p2.y + d);
	}
	glEnd();

}

bool LayeringModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	mouse_pos.x = (local_mouse.x / size().x + 0.5f) * width;
	mouse_pos.y = (local_mouse.y / size().y + 0.5f) * height;
/*
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		if (total_frames) {
			update_anim_tags();
		} else {
			update_regions();
		}
	}
*/
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_t) {
		show_tags = !show_tags;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_a) {
		allow_fail = !allow_fail;
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
		update_image();
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s) {
		update_image();
		save_png("ll_dump.png", width, height, &image[0]);
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
		unsigned int delta = (event.key.keysym.mod & KMOD_SHIFT)?10:1;
		if (current_frame - delta < total_frames) {
			set_frame(current_frame-delta);
		}
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT) {
		unsigned int delta = (event.key.keysym.mod & KMOD_SHIFT)?10:1;
		if (current_frame + delta < total_frames) {
			set_frame(current_frame+delta);
		}
		return true;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		for (unsigned int b = 0; b < bins.size(); ++b) {
			Box2f box = bins[b].get_box(dialog_tween);
			if (box.contains(mouse_pos)) {
				if (bins[b].layer >= layers.size()) continue;
				bool up = mouse_pos.y > box.center().y;
				if (dialog_point.x < width && dialog_point.y < height && tags.size() == width * height) {
					unsigned int t = tags[dialog_point.y * width + dialog_point.x];
					assert(t < stacking.lists.size());
					for (unsigned int i = 0; i < stacking.lists[t].size(); ++i) {
						if (stacking.lists[t][i] == bins[b].layer) {
							if (up && i + 1 < stacking.lists[t].size()) {
								stacking.flip_rel(t, bins[b].layer, stacking.lists[t][i+1], true);
								update_image();
							}
							if (!up && i - 1 < stacking.lists[t].size()) {
								stacking.flip_rel(t, bins[b].layer, stacking.lists[t][i-1], false);
								update_image();
							}
							break;
						}
					}
				}
				return true;
			}
		}
		{
			unsigned int r = (int)(mouse_pos.y + 0.5f);
			unsigned int c = (int)(mouse_pos.x + 0.5f);
			if (event.button.button == SDL_BUTTON_LEFT) {
				if (c < width && r < height) {
					dialog_point.x = c;
					dialog_point.y = r;
					update_dialog();
				}
			} else if (event.button.button == SDL_BUTTON_RIGHT) {
				dialog_point.x = (unsigned)-1;
				dialog_point.y = (unsigned)-1;
				update_dialog();
			}
		}
		return true;
	}
	if (event.type == SDL_MOUSEMOTION) {
		if (event.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
			if (model.verts.empty()) {
				acc_move_x += event.motion.xrel;
				acc_move_y -= event.motion.yrel;
			} else {
				float camera_rad = length(target - new_camera);
				Vector3f camera_in = normalize(target - new_camera);
				Vector3f camera_up = make_vector(0.0f, 1.0f, 0.0f);
				camera_up = normalize(camera_up - camera_in * (camera_in * camera_up));
				Vector3f camera_right = cross_product(camera_in, camera_up);
				new_camera += camera_right * camera_rad * event.motion.xrel / 400.0f;
				new_camera += camera_up *-camera_rad * event.motion.yrel / 400.0f;
			}
		} else {
			unsigned int r = (int)(mouse_pos.y + 0.5f);
			unsigned int c = (int)(mouse_pos.x + 0.5f);
			if (r < height && c < width && tags.size() == width * height) {
				unsigned int t = tags[r * width + c];
				assert(t < stacking.lists.size());
				if (stacking.lists[t].empty()) {
					current_layer = (unsigned)-1;
				} else {
					current_layer = stacking.lists[t].back();
				}
			}
		}
	}
	return false;
}

void LayeringModule::update(float elapsed_time) {
	//maybe allows scrubbing an animation?
	if (anim_pos() != last_anim_pos) {
		last_anim_pos = anim_pos();
		int next_frame = int(total_frames * last_anim_pos);
		if (next_frame >= int(total_frames)) next_frame = int(total_frames)-1;
		if (next_frame < 0) next_frame = 0;
		if (next_frame != int(current_frame)) {
			set_frame(next_frame);
		}
	}

	if (!model.verts.size() && !total_frames && current_layer < layers.size() && (acc_move_x != 0 || acc_move_y != 0)) {
		dialog_point.x = dialog_point.y = (unsigned)-1;
		update_dialog();
		int roll_x = ((acc_move_x % (int)width) + (int)width) % (int)width;
		int roll_y = ((acc_move_y % (int)height) + (int)height) % (int)height;
		acc_move_x = 0;
		acc_move_y = 0;
		vector< uint32_t > temp = layers[current_layer];
		for (unsigned int r = 0; r < height; ++r) {
			for (unsigned int c = 0; c < width; ++c) {
				unsigned int i = ((r + roll_y) % height) * width + (c + roll_x) % width;
				layers[current_layer][i] = temp[r*width+c];
			}
		}
		assert(textures.size() > current_layer);
		if (allow_fail) {
			update_regions(true);
		} else {
			vector< uint32_t > old_bits = bits;
			vector< uint32_t > old_tags = tags;
			ListGraph old_stacking = stacking;
			if (!update_regions(true)) {
				layers[current_layer] = temp;
				bits = old_bits;
				tags = old_tags;
				stacking = old_stacking;
				assert(update_regions(true));
			}
		}
		//update texture:
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures[current_layer]);
		upload_texture(width, height, layers[current_layer]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	} else {
		acc_move_x = acc_move_y = 0;
	}
	if (new_camera != camera) {
		Vector3f old_camera = camera;
		camera = new_camera;
		model_dirty = true;
		if (allow_fail) {
			update_regions();
		} else {
			vector< uint32_t > old_bits = bits;
			vector< uint32_t > old_tags = tags;
			ListGraph old_stacking = stacking;
			if (!update_regions(true)) {
				bits = old_bits;
				tags = old_tags;
				stacking = old_stacking;
				camera = old_camera;
				model_dirty = true;
				assert(update_regions(true)); //we can try?
			}
		}
	}
	if (model_dirty) {
		update_regions();
	}
}

class BinCompare {
public:
	bool operator()(DialogBin const &a, DialogBin const &b) const {
		bool saw_b = false;
		bool saw_a = false;
		for (unsigned int i = 0; i < list.size(); ++i) {
			if (list[i] == a.layer) {
				if (saw_b) return false;
				saw_a = true;
			}
			if (list[i] == b.layer) {
				if (saw_a) return true;
				saw_b = true;
			}
		}
		if (saw_a) return false;
		if (saw_b) return true;
		return false;
	}
	vector< unsigned int > list;
};

void LayeringModule::update_dialog() {
	//LOG_INFO("Updating for " << (int)dialog_point.x << ", " << (int)dialog_point.y << ".");
	if (dialog_point.x < width && dialog_point.y < height && tags.size() == width * height) {
		Box2i tag_box;
		tag_box.min = make_vector(0,0);
		tag_box.max = make_vector(width,height);
		if (bits.size() == tags.size() && bits[dialog_point.y * width + dialog_point.x]) { //find box.
			unsigned int tag = tags[dialog_point.y * width + dialog_point.x];
			vector< Vector2i > to_visit;
			vector< bool > visited(width * height, false);
			tag_box.min = tag_box.max = make_vector((int)dialog_point.x, (int)dialog_point.y);
			to_visit.push_back(tag_box.min);
			visited[dialog_point.y * width + dialog_point.x] = true;
			const unsigned int Ofs = 4;
			static Vector2i ofs[Ofs] = {
				{{{-1, 0}}},
				{{{ 1, 0}}},
				{{{ 0,-1}}},
				{{{ 0, 1}}},
			};
			while (!to_visit.empty()) {
				Vector2i p = to_visit.back();
				to_visit.pop_back();
				if (tags[p.y * width + p.x] != tag) continue;
				tag_box.min = min(tag_box.min, p);
				tag_box.max = max(tag_box.max, p);
				for (unsigned int i = 0; i < Ofs; ++i) {
					Vector2i n = p + ofs[i];
					if ((unsigned)(n.x) < width && (unsigned)(n.y) < height) {
						unsigned int i = n.y * width + n.x;
						if (!visited[i]) {
							visited[i] = true;
							to_visit.push_back(n);
						}
					}
				}
			}
		}
		dialog_box = tag_box;
		BinCompare bc;
		bc.list = stacking.lists[tags[dialog_point.y * width + dialog_point.x]];
		sort(bins.begin(), bins.end(), bc);
		for (unsigned int b = 0; b < bins.size(); ++b) {
			bins[b].old_pos = bins[b].get_pos(dialog_tween);
			bins[b].old_show = bins[b].get_show(dialog_tween, make_vector(width * 0.5f, height * 0.5f, width * 0.5f));
			bins[b].active = false;
			for (unsigned int i = 0; i < bc.list.size(); ++i) {
				if (bc.list[i] == bins[b].layer) {
					bins[b].active = true;
					break;
				}
			}
			bins[b].new_pos = make_vector(0.0f,-(0.5f * bins.size() - 0.5f - b) * BinHeight / BinWidth);
			if (bins[b].active) {
				bins[b].new_show.x = 0.5f * (tag_box.min.x + tag_box.max.x);
				bins[b].new_show.y = 0.5f * (tag_box.min.y + tag_box.max.y);
				bins[b].new_show.z = 0.5f * tag_box.size().x;
				if (tag_box.size().y > tag_box.size().x) {
					bins[b].new_show.z = 0.5f * tag_box.size().y;
				}
				bins[b].new_show.z += 5.0f;
			} else {
				bins[b].new_show = make_vector(width * 0.5f, height * 0.5f, width * 0.5f);
			}
		}
	} else {
		for (unsigned int b = 0; b < bins.size(); ++b) {
			bins[b].old_pos = bins[b].get_pos(dialog_tween);
			bins[b].new_pos = make_vector(0.0f,-(0.5f * bins.size() - 0.5f - b) * BinHeight / BinWidth);
			bins[b].active = true;
			bins[b].old_show = bins[b].get_show(dialog_tween, make_vector(width * 0.5f, height * 0.5f, width * 0.5f));
			bins[b].new_show = make_vector(width * 0.5f, height * 0.5f, width * 0.5f);
		}
	}
	dialog_tween = 0.0f;
}

void LayeringModule::set_frame(unsigned int new_frame) {
	if (new_frame == current_frame) return;
	if (total_frames == 0) return;
	current_frame = new_frame;
	assert(anim_layers.size() == layers.size());
	for (unsigned int l = 0; l < layers.size(); ++l) {
		assert(!anim_layers[l].empty());
		layers[l] = anim_layers[l][current_frame % anim_layers[l].size()];
		//new frame --> new texture.
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures[l]);
		upload_texture(width, height, layers[l]);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	if (anim_tags.size() == total_frames) {
		tags = anim_tags[current_frame];
		tags_dirty = true;
	}
	update_image();
	update_dialog();
}

void LayeringModule::update_anim_tags() {
	ListGraph anim_graph;
	vector< uint32_t > frame_bits(width*height, 0);
	vector< uint32_t > last_frame_bits(width*height);
	anim_tags.clear();
	vector< vector< uint32_t > > tag_maps;
	for (unsigned int frame = 0; frame < total_frames; ++frame) {
		LOG_INFO("Doing frame " << frame << " of " << total_frames);
		last_frame_bits.swap(frame_bits); //keep around the bits from last frame.
		//zero bits:
		memset(&(frame_bits[0]), '\0', sizeof(uint32_t)*width*height);
		//code each region:
		for (unsigned int l = 0; l < anim_layers.size(); ++l) {
			assert(!anim_layers[l].empty());
			uint32_t mask = 1 << l;
			vector< uint32_t > const &layer = anim_layers[l][frame % anim_layers[l].size()];
			assert(layer.size() == frame_bits.size());
			for (unsigned int i = 0; i < layer.size(); ++i) {
				if (layer[i] & 0xff000000) {
					frame_bits[i] |= mask;
				}
			}
		}
		//extract:
		ListGraph frame_graph;
		vector< uint32_t > frame_tags;
		extract_tags(width, height, layers.size(), frame_bits, frame_tags, frame_graph);
		tags_dirty = true;
		LOG_INFO("Extracted " << frame_graph.lists.size() << " components.");
		anim_tags.push_back(frame_tags);
		if (frame == 0) {
			//First we just store:
			anim_graph = frame_graph;
			tag_maps.push_back(vector< uint32_t >(frame_graph.lists.size()));
			for (unsigned int i = 0; i < tag_maps.back().size(); ++i) {
				tag_maps.back()[i] = i;
			}
		} else {
			//second and beyond have to do correspondances:
			tag_maps.push_back(vector< uint32_t >(frame_graph.lists.size()));
			//tag map says that these are all at end of anim_graph:
			for (unsigned int i = 0; i < tag_maps.back().size(); ++i) {
				tag_maps.back()[i] = anim_graph.lists.size() + i;
			}
			//frame graph edges updated as well:
			for (unsigned int t = 0; t < frame_graph.edges.size(); ++t) {
				for (unsigned int e = 0; e < frame_graph.edges[t].size(); ++e) {
					frame_graph.edges[t][e] += anim_graph.lists.size();
				}
			}
			//frame graph added to anim_graph:
			anim_graph.lists.insert(anim_graph.lists.end(), frame_graph.lists.begin(), frame_graph.lists.end());
			anim_graph.edges.insert(anim_graph.edges.end(), frame_graph.edges.begin(), frame_graph.edges.end());
			//ooookay, so now:
			vector< uint32_t > const &last_frame_tags = anim_tags[anim_tags.size()-2];
			for (unsigned int pos = 0; pos < frame_tags.size(); ++pos) {
				uint32_t tag_a = tag_maps[frame][frame_tags[pos]];
				uint32_t tag_b = tag_maps[frame-1][last_frame_tags[pos]];
				if (tag_a == tag_b) continue;
				{ //DEBUG: check that bits match lists [tag_a / frame]
					unsigned int total = 0;
					for (unsigned int l = 0; l < layers.size(); ++l) {
						if ( (1 << l) & frame_bits[pos] ) {
							++total;
						}
					}
					assert(total == anim_graph.lists[tag_a].size());
					for (unsigned int i = 0; i < anim_graph.lists[tag_a].size(); ++i) {
						assert( (1 << anim_graph.lists[tag_a][i]) & frame_bits[pos] );
					}
				}
				{ //DEBUG: check that bits match lists [tag_b / last_frame]
					unsigned int total = 0;
					for (unsigned int l = 0; l < layers.size(); ++l) {
						if ( (1 << l) & last_frame_bits[pos] ) {
							++total;
						}
					}
					assert(total == anim_graph.lists[tag_b].size());
					for (unsigned int i = 0; i < anim_graph.lists[tag_b].size(); ++i) {
						assert( (1 << anim_graph.lists[tag_b][i]) & last_frame_bits[pos] );
					}
				}

				if (frame_bits[pos] != last_frame_bits[pos]) {
					//ok, just make 'em adjacent...
					bool found = false;
					for (unsigned int i = 0; i < anim_graph.edges[tag_a].size(); ++i) {
						if (anim_graph.edges[tag_a][i] == tag_b) {
							found = true;
							break;
						}
					}
					if (!found) {
						//better not exist in tag_b already somehow:
						for (unsigned int i = 0; i < anim_graph.edges[tag_b].size(); ++i) {
							assert(anim_graph.edges[tag_b][i] != tag_a);
						}
						//add edge:
						anim_graph.edges[tag_a].push_back(tag_b);
						anim_graph.edges[tag_b].push_back(tag_a);
					}
					continue;
				}
				//Ok, we have two different tags that need to be unified.
				// ... OR: could just make 'em adjacent.
				if (tag_a > tag_b) swap(tag_a, tag_b);
				/*LOG_INFO("At " << pos << ", Unifying " << tag_a << " and " << tag_b);
				//DEBUG:
				LOG_INFO("Bits are " << frame_bits[pos] << " and " << last_frame_bits[pos]);
				LOG_INFO(tag_a << " has:");
				for (unsigned int i = 0; i < anim_graph.lists[tag_a].size(); ++i) {
					LOG_INFO("   " << anim_graph.lists[tag_a][i]);
				}
				LOG_INFO(tag_b << " has:");
				for (unsigned int i = 0; i < anim_graph.lists[tag_b].size(); ++i) {
					LOG_INFO("   " << anim_graph.lists[tag_b][i]);
				}*/
				//change tag maps:
				for (unsigned int f = 0; f < tag_maps.size(); ++f) {
					for (unsigned int t = 0; t < tag_maps[f].size(); ++t) {
						if (tag_maps[f][t] == tag_b) {
							tag_maps[f][t] = tag_a;
						}
						if (tag_maps[f][t] > tag_b) {
							tag_maps[f][t] -= 1;
						}
					}
				}
				//unify in anim_graph:
				for (unsigned int t = 0; t < anim_graph.edges.size(); ++t) {
					for (unsigned int e = 0; e < anim_graph.edges[t].size(); ++e) {
						if (anim_graph.edges[t][e] == tag_b) {
							anim_graph.edges[t][e] = tag_a;
						}
						if (anim_graph.edges[t][e] > tag_b) {
							anim_graph.edges[t][e] -= 1;
						}
					}
				}
				//check lists:
				assert(anim_graph.lists[tag_a].size() == anim_graph.lists[tag_b].size());
				for (unsigned int i = 0; i < anim_graph.lists[tag_a].size(); ++i) {
					assert(anim_graph.lists[tag_a][i] == anim_graph.lists[tag_b][i]);
				}
				//delete list:
				anim_graph.lists.erase(anim_graph.lists.begin() + tag_b);
				//unify edges:
				for (unsigned int i = 0; i < anim_graph.edges[tag_b].size(); ++i) {
					bool found = false;
					for (unsigned int j = 0; j < anim_graph.edges[tag_a].size(); ++j) {
						if (anim_graph.edges[tag_a][j] == anim_graph.edges[tag_b][i]) {
							found = true;
							break;
						}
					}
					if (!found) {
						anim_graph.edges[tag_a].push_back(anim_graph.edges[tag_b][i]);
					}
				}
				//delete edges for tag_b:
				anim_graph.edges.erase(anim_graph.edges.begin() + tag_b);
				assert(anim_graph.edges.size() == anim_graph.lists.size());
				assert(anim_graph.is_consistent());
			}
		}
		LOG_INFO(" -> " << anim_graph.lists.size() << " total, after unification.");
	}
	LOG_INFO("Remapping...");
	assert(tag_maps.size() == anim_tags.size());
	for (unsigned int frame = 0; frame < anim_tags.size(); ++frame) {
		for (unsigned int i = 0; i < anim_tags[frame].size(); ++i) {
			assert(anim_tags[frame][i] < tag_maps[frame].size());
			anim_tags[frame][i] = tag_maps[frame][anim_tags[frame][i]];
			assert(anim_tags[frame][i] < anim_graph.lists.size());
		}
	}
	stacking = anim_graph;
}

void LayeringModule::add_model(string filename) {
	LOG_INFO("Here we go with '" << filename << "'");
	model.clear();
	if (!load_obj(filename, model)) {
		LOG_ERROR("Could not load model.");
		model.clear();
	}
	camera = model.bounds.max + (model.bounds.max - model.bounds.center()) * 0.5f;
	target = model.bounds.center();
	model_dirty = true;
	update_regions();
}

void LayeringModule::render_model() {
	if (width == 0 || height == 0) {
		LOG_WARNING("render_model called with no-sized screen.");
		return;
	}
	model_dirty = false;
	Graphics::ProgramObjectRef nopeel_shader = NULL;
	Graphics::ProgramObjectRef peel_shader = NULL;
	if (nopeel_shader.ref == NULL || peel_shader.ref == NULL) {
		Graphics::ShaderObjectRef vert = Graphics::get_shader_object("ll_shaders/shade_model_vert.glsl", GL_VERTEX_SHADER_ARB);
		assert(vert.ref);
		Graphics::ShaderObjectRef frag = Graphics::get_shader_object("ll_shaders/peel_model_frag.glsl", GL_FRAGMENT_SHADER_ARB);
		assert(frag.ref);
		nopeel_shader = Graphics::get_program_object(vert);
		peel_shader = Graphics::get_program_object(vert, frag);

		assert(nopeel_shader.ref);
		assert(peel_shader.ref);

		glUseProgramObjectARB(peel_shader->handle);
		glUniform1iARB(glGetUniformLocationARB(peel_shader->handle, "max_depth"), 0);
		glUseProgramObjectARB(0);
	}
	static GLuint model_fb = 0;
	static GLuint depth_tex = 0;
	static GLuint last_depth_tex = 0;
	if (model_fb == 0) {
		glGenFramebuffersEXT(1, &model_fb);
	}
	if (depth_tex == 0) {
		glGenTextures(1, &depth_tex);
	}
	{ //set up depth_tex:
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, depth_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	if (last_depth_tex == 0) {
		glGenTextures(1, &last_depth_tex);
	}
	{ //set up last_depth_tex:
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, last_depth_tex);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, model_fb);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Vector3f to = normalize(target - camera);
	float z_center = (model.bounds.center() - camera) * to;
	float z_size = fabsf(model.bounds.size().x * to.x)
		+ fabsf(model.bounds.size().y * to.y)
		+ fabsf(model.bounds.size().z * to.z);
	float s = 2.0f / length(model.bounds.size());
	glTranslatef(0.0f, 0.0f, 0.5f);
	glScalef(s * height / width, s, 1.0f / z_size);
	glTranslatef(0.0f, 0.0f, z_center);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt( camera.x, camera.y, camera.z, target.x, target.y, target.z, 0, 1, 0 );
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	unsigned int layer = 0;
	while (1) {
		while (layer >= model_layers.size()) {
			model_layers.push_back(0);
			glGenTextures(1, &(model_layers.back()));
		}
		{ //(re-)allocate width/height:
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, model_layers[layer]);
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			Graphics::gl_errors("tex setup");
		}

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, model_layers[layer], 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_RECTANGLE_ARB, depth_tex, 0);
		Graphics::gl_errors("fb setup");
		{ //check:
			GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
			if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
				LOG_WARNING("WARNING: FRAMEBUFFER not complete!");
			}
		}
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (layer == 0) {
			glUseProgramObjectARB(nopeel_shader->handle);
		} else {
			glUseProgramObjectARB(peel_shader->handle);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, last_depth_tex);
		}
		glBegin(GL_QUADS);
		model.emit_as_quads();
		glEnd();
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glUseProgramObjectARB(0);
		swap(depth_tex, last_depth_tex);
		++layer;
		if (layer > 3) break; //DEBUG: peel only a bit, should use occlusion query.
	}
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	//clean up extra textures:
	while (model_layers.size() > layer) {
		glDeleteTextures(1, &(model_layers.back()));
		model_layers.pop_back();
	}
	assert(textures.size() == layers.size());
	while (layers.size() > fixed_layers + layer) {
		layers.pop_back();
		textures.pop_back();
	}
	{ //clean up dialog bins:
		//get rid of old ones.
		for (unsigned int b = 0; b < bins.size();) {
			if (bins[b].layer >= layers.size()) {
				bins.erase(bins.begin() + b);
				continue;
			}
			if (bins[b].layer >= fixed_layers) {
				bins[b].name = string("depth") + (char)('0' + bins[b].layer - fixed_layers);
			}
			++b;
		}
		//add ones:
		while (bins.size() < layers.size()) {
			bins.push_back(DialogBin(bins.size(), string("depth") + (char)('0' + bins.size() - fixed_layers)));
		}
	}
	for (unsigned int i = 0; i < model_layers.size(); ++i) {
		while (layers.size() <= fixed_layers + i) {
			layers.push_back(vector< uint32_t >());
			textures.push_back(0);
		}
		if (layers[fixed_layers+i].size() != width * height) {
			layers[fixed_layers+i].resize(width*height);
		}
		unsigned int inv = model_layers.size() - 1 - i;
		textures[fixed_layers + i] = model_layers[inv];
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, model_layers[inv]);
		glGetTexImage(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(layers[fixed_layers+i][0]));
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	LOG_INFO("Model Layers: " << model_layers.size() << "; Layers: " << layers.size());
	update_dialog();
	Graphics::gl_errors("render_model");
}
