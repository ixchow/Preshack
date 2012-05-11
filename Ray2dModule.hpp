#ifndef RAY2DMODULE_HPP
#define RAY2DMODULE_HPP
#include "Module.hpp"

#include <deque>
#include <limits>

using std::pair;
using std::deque;
using std::numeric_limits;

class Ray2d {
public:
	Ray2d(Vector2f _s, Vector2f _d, Vector4f _color, int _type = LIGHT, unsigned int _parent = (unsigned)-1)
		: s(_s), d(_d),
		intersection(numeric_limits< float >::infinity()),
		normal(make_vector(0.0f,0.0f)),
		color(_color), type(_type), parent(_parent), children(0), material(0) {
	}
	Vector2f s;
	Vector2f d;
	float intersection;
	Vector2f normal;
	Vector4f color;
	enum { LIGHT, SHADOW, VIEW };
	int type;
	unsigned int parent;
	unsigned int children;
	unsigned int material;
};

class Ray2dModule : public Module {
public:
	Ray2dModule() {
		//out_ports.push_back(&accum);
		//accum.position = make_vector(0.25f, 0.0f);
		//accum() = 0.0f;
		view_center = make_vector(0.0f, 0.0f);
		view_radius = make_vector(1.0f, 1.0f);
		camera_center = make_vector(0.7f, 0.0f);
		camera_direction = make_vector(-1.0f, 0.0f);
		camera_fov = 60.0f / 180.0f * float(M_PI);

		light_center = make_vector(0.5f, 0.9f);

		spheres.push_back(make_vector(0.0f,-0.1f, 0.2f));
		planes.push_back(make_vector(0.0f, 1.0f, -0.9f));
		lowest_untraced = 0;
	}
	virtual ~Ray2dModule() {
	}
	//Item functions:
	virtual Vector2f size();
	virtual void draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse = 0);
	//Module functions:
	virtual void update(float elapsed_time);
	virtual bool handle_event(SDL_Event const &event, Vector2f local_mouse);

	void continue_ray(unsigned int r);
	Vector3f material_color(unsigned int ind);
	float material_reflectance(unsigned int ind);

	Vector2f view_center;
	Vector2f view_radius;

	Vector2f camera_center;
	Vector2f camera_direction;
	float camera_fov;

	Vector2f light_center;

	vector< Vector3f > spheres;
	vector< Vector3f > planes;

	vector< Ray2d > rays;
	unsigned int lowest_untraced;
	
	//PortDataContainer< TexPacket > tex;
};

#endif //RAY2DMODULE_HPP
