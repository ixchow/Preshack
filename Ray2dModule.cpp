#include "Ray2dModule.hpp"

#include <Graphics/Graphics.hpp>
#include <Graphics/Font.hpp>
#include <Vector/VectorGL.hpp>

const float CamSize = 0.2f;

using std::make_pair;

namespace {
	Module *create_module(const std::string &params) {
		Ray2dModule *p = new Ray2dModule();
		//istringstream ps(params);
		//Vector3f col;
		//if (ps >> col) p->set_color(col);
		return p;
	}
	class Fred {
	public:
		Fred() {
			register_module("ray2d", create_module);
		}
	} fred;
}

Vector2f Ray2dModule::size() {
	return make_vector(2.0f, 2.0f / view_radius.x * view_radius.y);
}

void Ray2dModule::draw(Box2f viewport, Box2f screen_viewport, float scale, unsigned int recurse) {

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glBoxToBox(viewport, screen_viewport);

	Vector2f my_size = size();
	{ //background:
		glBegin(GL_QUADS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex2f(-0.5f * my_size.x, -0.5f * my_size.y);
		glVertex2f( 0.5f * my_size.x, -0.5f * my_size.y);
		glVertex2f( 0.5f * my_size.x,  0.5f * my_size.y);
		glVertex2f(-0.5f * my_size.x,  0.5f * my_size.y);
		glEnd();
	}

	glPushAttrib(GL_SCISSOR_BIT);

	{ //enable clipping:
		int min_x = int((0.5f / Graphics::aspectf() * screen_viewport.min.x + 0.5f) * Graphics::screen_x + 0.5f);
		int max_x = int((0.5f / Graphics::aspectf() * screen_viewport.max.x + 0.5f) * Graphics::screen_x + 0.5f);
		int min_y = int((0.5f * screen_viewport.min.y + 0.5f) * Graphics::screen_y + 0.5f);
		int max_y = int((0.5f * screen_viewport.max.y + 0.5f) * Graphics::screen_y + 0.5f);
		if (min_x < 0) min_x = 0;
		if (max_x > Graphics::screen_x) max_x = Graphics::screen_x;
		if (min_y < 0) min_y = 0;
		if (max_y > Graphics::screen_y) max_y = Graphics::screen_y;
		glScissor(min_x, min_y, max_x - min_x, max_y - min_y);
	}
	glEnable(GL_SCISSOR_TEST);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(0.5f * my_size.x / view_radius.x, 0.5f * my_size.y / view_radius.y, 1.0f);
	glTranslatef(-view_center.x, -view_center.y, 0.0f);

	//draw the "world":
	//spheres:
	for (unsigned int s = 0; s < spheres.size(); ++s) {
		glColor4f(1.0f, 0.5f, 0.5f, 0.5f);
		glBegin(GL_TRIANGLE_FAN);
		const unsigned int Angles = 32;
		for (unsigned int a = 0; a < Angles; ++a) {
			float ang = a / float(Angles) * float(M_PI) * 2.0f;
			glVertex2f(cosf(ang) * spheres[s].z + spheres[s].x , sinf(ang) * spheres[s].z + spheres[s].y);
		}
		glEnd();
		glColor3f(1.0f, 0.5f, 0.5f);
		glBegin(GL_LINE_LOOP);
		for (unsigned int a = 0; a < Angles; ++a) {
			float ang = a / float(Angles) * float(M_PI) * 2.0f;
			glVertex2f(cosf(ang) * spheres[s].z + spheres[s].x , sinf(ang) * spheres[s].z + spheres[s].y);
		}
		glEnd();
	}
	//planes:
	float r = length(view_radius);
	for (unsigned int p = 0; p < planes.size(); ++p) {
		const float PlaneSize = 0.07f;
		Vector2f c = view_center + (planes[p].z - view_center * planes[p].xy) * planes[p].xy;
		glBegin(GL_QUADS);
		glColor4f(1.0f, 0.5f, 0.5f, 0.5f);
		glVertex(c - r * perpendicular(planes[p].xy));
		glVertex(c + r * perpendicular(planes[p].xy));
		glColor4f(1.0f, 0.5f, 0.5f, 0.0f);
		glVertex(c + r * perpendicular(planes[p].xy) - PlaneSize * planes[p].xy);
		glVertex(c - r * perpendicular(planes[p].xy) - PlaneSize * planes[p].xy);
		glEnd();
		glBegin(GL_LINES);
		glColor3f(1.0f, 0.5f, 0.5f);
		glVertex(c - r * perpendicular(planes[p].xy));
		glVertex(c + r * perpendicular(planes[p].xy));
		glEnd();
	}
	{//rays:
		float view_r = length(view_radius);
		for (unsigned int r = 0; r < rays.size() && r < lowest_untraced; ++r) {
			Vector2f a = rays[r].s;
			float t = rays[r].intersection;
			if (t > 2.0f * view_r) t = 2.0f * view_r;
			Vector2f b = rays[r].s + rays[r].d * t;
			glBegin(GL_LINES);
			glColor(rays[r].color); glVertex(a); glVertex(b);
			glEnd();
			if (rays[r].type == Ray2d::VIEW) {
				glLineStipple(1,0x0003);
				glLineWidth(3.0f);
				glEnable(GL_LINE_STIPPLE);
				glColor3f(0.7f, 0.7f, 0.7f);
				glBegin(GL_LINES);
				glVertex(a); glVertex(b);
				glEnd();
				glDisable(GL_LINE_STIPPLE);
				glLineWidth(1.0f);
			} else if (rays[r].type == Ray2d::SHADOW) {
				glLineStipple(1,0x0303);
				glLineWidth(3.0f);
				glEnable(GL_LINE_STIPPLE);
				glColor3f(0.7f, 0.7f, 0.7f);
				glBegin(GL_LINES);
				glVertex(a); glVertex(b);
				glEnd();
				glDisable(GL_LINE_STIPPLE);
				glLineWidth(1.0f);
			} else {
			}
		}
		glLineStipple(1,0xFFFF);
	}
	//light:
	{
		const float LightSize = 0.05f;
		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 0.7f);
		glVertex(light_center + LightSize*r*make_vector( 0.0f,-1.0f));
		glVertex(light_center + LightSize*r*make_vector( 0.0f, 1.0f));
		glVertex(light_center + LightSize*r*make_vector(-1.0f, 0.0f));
		glVertex(light_center + LightSize*r*make_vector( 1.0f, 0.0f));
		glVertex(light_center + LightSize*r*make_vector(-0.7f,-0.7f));
		glVertex(light_center + LightSize*r*make_vector( 0.7f, 0.7f));
		glVertex(light_center + LightSize*r*make_vector(-0.7f, 0.7f));
		glVertex(light_center + LightSize*r*make_vector( 0.7f,-0.7f));
		glEnd();
	}
	//camera:
	{
		glBegin(GL_LINES);
		float w = tanf(camera_fov * 0.5f);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex(camera_center + CamSize * r * (camera_direction + w * perpendicular(camera_direction)));
		glVertex(camera_center + CamSize * r * (camera_direction - w * perpendicular(camera_direction)));
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		glVertex(camera_center + CamSize * r * (camera_direction + w * perpendicular(camera_direction)));
		glVertex(camera_center);
		glVertex(camera_center + CamSize * r * (camera_direction - w * perpendicular(camera_direction)));
		glVertex(camera_center);
		glEnd();
	}

	glPopMatrix();

	glPopAttrib();

	{ //outline:
		glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_LINE_LOOP);
		glVertex2f(-0.5f * my_size.x, -0.5f * my_size.y);
		glVertex2f( 0.5f * my_size.x, -0.5f * my_size.y);
		glVertex2f( 0.5f * my_size.x,  0.5f * my_size.y);
		glVertex2f(-0.5f * my_size.x,  0.5f * my_size.y);
		glEnd();
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	Graphics::gl_errors("Ray2d::draw");

}

void Ray2dModule::update(float elapsed_time) {
	unsigned int Limit = 1000;
	unsigned int first = lowest_untraced;
	unsigned int second = rays.size();
	if (second > first + Limit) {
		second = first + Limit;
	}
	lowest_untraced = second;

	//Ok, let's trace!
	const float Close = 0.01f;
	for (unsigned int s = 0; s < spheres.size(); ++s) {
		Vector2f center = spheres[s].xy;
		float radius_squared = spheres[s].z * spheres[s].z;
		for (unsigned int r = first; r < second; ++r) {
			Ray2d &ray = rays[r];
			float a = ray.d.x * ray.d.x + ray.d.y * ray.d.y;
			float b = 2.0f * (ray.d.x * (ray.s.x - center.x) + ray.d.y * (ray.s.y - center.y));
			float c = length_squared(ray.s - center) - radius_squared;
			//( -b +- sqrt(b^2 - 4*a*c) ) / 2a
			float d = b*b - 4.0f*a*c;
			if (d < 0.0f) continue;
			float root = sqrtf(d);
			float t1 = (-b - root) / (2.0f * a);
			float t2 = (-b + root) / (2.0f * a);
			if (t1 > Close && t1 < ray.intersection) {
				ray.intersection = t1;
				ray.normal = (t1 * ray.d + ray.s - center) / spheres[s].z;
			} else if (t2 > Close && t2 < ray.intersection) {
				ray.intersection = t2;
				ray.normal = (t2 * ray.d + ray.s - center) / spheres[s].z;
			}
		}
	}
	for (unsigned int p = 0; p < planes.size(); ++p) {
		Vector2f normal = planes[p].xy;
		float offset = planes[p].z;
		for (unsigned int r = first; r < second; ++r) {
			Ray2d &ray = rays[r];
			float d = normal * rays[r].s - offset;
			float a = normal * rays[r].d;
			if (d < 0.0f || a >= 0.0) continue;
			float t = -d/a;
			if (t > Close && t < ray.intersection) {
				ray.intersection = t;
				ray.normal = normal;
			}
		}
	}
}

bool Ray2dModule::handle_event(SDL_Event const &event, Vector2f local_mouse) {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
		unsigned int max = rays.size();
		if (max > lowest_untraced) max = lowest_untraced;
		for (unsigned int r = max - 1; r < rays.size(); --r) {
			if (rays[r].type == Ray2d::SHADOW) {
				Vector2f to_light = light_center - rays[r].s;
				float amt = 1.0f;
				if (rays[r].intersection != length(to_light)) {
					amt = 0.0f;
				}
				rays[r].color = make_vector(1.0f, 1.0f, 1.0f, amt);
				if (rays[r].parent < rays.size()) {
					rays[rays[r].parent].color.xyz += material_color(rays[rays[r].parent].material) * (rays[rays[r].parent].normal * rays[r].d) * rays[r].color.w;
					if (rays[rays[r].parent].children == 0) {
						std::cout << "Oddly, no proper child count." << std::endl;
					} else {
						rays[rays[r].parent].children -= 1;
					}
					rays[r].parent = (unsigned)-1; //we've already contributed.
					
				}
			}
			if (rays[r].type == Ray2d::VIEW && rays[r].children == 0) {
				if (rays[r].parent >= rays.size()) continue;

				rays[rays[r].parent].color.xyz += rays[r].color.xyz * material_reflectance(rays[rays[r].parent].material);
				if (rays[rays[r].parent].children == 0) {
					std::cout << "Oddly, no proper child count." << std::endl;
				} else {
					rays[rays[r].parent].children -= 1;
				}
				rays[r].parent = (unsigned)-1; //we've already contributed.
			}
		}
		return true;
	}
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
		unsigned int max = rays.size();
		if (max > lowest_untraced) max = lowest_untraced;
		for (unsigned int r = 0; r < lowest_untraced; ++r) {
			if (rays[r].children) continue;
			continue_ray(r);
		}
		return true;
	}

	Vector2f view_mouse;
	view_mouse.x = local_mouse.x / size().x * 2.0f * view_radius.x + view_center.x;
	view_mouse.y = local_mouse.y / size().y * 2.0f * view_radius.y + view_center.y;
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		float thresh = length_squared(view_radius) * 0.05f * 0.05f;
		if (length_squared(light_center - view_mouse) < thresh) {
			rays.clear();
			for (unsigned int a = 0; a < 16; ++a) {
				float ang = a / 16.0f * float(M_PI) * 2.0f;
				rays.push_back(Ray2d(light_center, make_vector(cosf(ang), sinf(ang)), make_vector(1.0f, 1.0f, 1.0f, 1.0f)));
			}
			lowest_untraced = 0;
			return true;
		}
		if (length_squared(camera_center - view_mouse) < thresh) {
			rays.clear();
			float w = tanf(camera_fov / 2.0f);
			for (unsigned int px = 0; px < 32; ++px) {
				float p = px / float(31);
				rays.push_back(Ray2d(camera_center, normalize(camera_direction + (p - 0.5f) * 2.0f * w * perpendicular(camera_direction)), make_vector(0.0f, 0.0f, 0.0f, 1.0f), Ray2d::VIEW));
			}
			lowest_untraced = 0;
			return true;
		}
		{
			float view_r = length(view_radius);
			float d = (view_mouse - camera_center) * perpendicular(camera_direction);
			float w = tanf(camera_fov / 2.0f) * CamSize * view_r;
			if (d < -w) d = -w;
			if (d > w) d = w;
			Vector2f lens_point = CamSize * view_r * camera_direction + perpendicular(camera_direction) * d;
			if (length_squared(lens_point + camera_center - view_mouse) < thresh) {
				if (!(SDL_GetModState() & KMOD_SHIFT)) {
					rays.clear();
				}
				lowest_untraced = rays.size();
				rays.push_back(Ray2d(camera_center, normalize(lens_point), make_vector(0.0f, 0.0f, 0.0f, 1.0f), Ray2d::VIEW));
				return true;
			}
		}
		unsigned int close_ray = (unsigned)-1;
		float ray_dis = thresh;
		for (unsigned int r = 0; r < rays.size() && r < lowest_untraced; ++r) {
			if (rays[r].intersection == numeric_limits< float >::infinity()) continue;
			float dis = length_squared(rays[r].s + rays[r].d * rays[r].intersection - view_mouse);
			if (dis < ray_dis) {
				ray_dis = dis;
				close_ray = r;
			}
		}
		if (close_ray < rays.size()) {
			continue_ray(close_ray);
			return true;
		}
	}
	return false;
}

void Ray2dModule::continue_ray(unsigned int close_ray) {
	Vector2f s = rays[close_ray].s + rays[close_ray].d * rays[close_ray].intersection;
	Vector2f normal = rays[close_ray].normal;
	Vector3f surf_color = make_vector(1.0f, 0.5f, 0.5f);
	if (rays[close_ray].type == Ray2d::LIGHT) {
		rays[close_ray].children = 1; //on light rays, just a 'has children' flag.
		Vector4f color = rays[close_ray].color;
		float att = normal * -rays[close_ray].d;
		//diffuse:
		for (unsigned int a = 1; a < 11; ++a) {
			float ang = a / 11.0f * float(M_PI); //half-circle.
			rays.push_back(Ray2d(s, normal * sinf(ang) + perpendicular(normal) * cosf(ang), make_vector(product(color.xyz, surf_color), color.w * att * sinf(ang))));
		}
		//reflection:
		rays.push_back(Ray2d(s, rays[close_ray].d - 2 * (rays[close_ray].d * normal) * normal, rays[close_ray].color));
	} else if (rays[close_ray].type == Ray2d::VIEW) {
		//shadowed?:
		Vector2f to_light = light_center - s;
		if (to_light * normal > 0) {
			rays.push_back(Ray2d(s, normalize(to_light), make_vector(surf_color, 0.0f), Ray2d::SHADOW, close_ray));
			rays.back().intersection = length(to_light);
			rays[close_ray].children += 1;
		}
		//reflection:
		rays.push_back(Ray2d(s, rays[close_ray].d - 2 * (rays[close_ray].d * normal) * normal, make_vector(0.0f, 0.0f, 0.0f, 0.5f), Ray2d::VIEW, close_ray));
		rays[close_ray].children += 1;
	}
}

Vector3f Ray2dModule::material_color(unsigned int ind) {
	if (ind == 0) {
		return make_vector(1.0f, 0.5f, 0.5f);
	} else {
		return make_vector(0.5f, 0.5f, 1.0f);
	}
}

float Ray2dModule::material_reflectance(unsigned int ind) {
	return 0.5f;
}
