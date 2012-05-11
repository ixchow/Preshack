#include "GP_Stroke.hpp"

void Stroke::clear() {
	points.clear();
	new_points = 0;
	dirty = false;
}

void Stroke::submit_point(Vector2f const &new_point) {
	points.push_back(new_point);
	++new_points;
	//TODO: 'new point' hook?
}

void SmoothStroke::clear() {
	smoothing_queue.clear();
	Stroke::clear();
}

void SmoothStroke::submit_point(Vector2f const &new_point) {
	//chop stroke, add points:
	if (points.empty() && smoothing_queue.empty()) {
		//no stroke yet, so dump starting point in:
		smoothing_queue.push_back(new_point);
		last = new_point;
		along = 0.0f;
	}
	while (along + length(new_point - last) > Step) {
		last += normalize(new_point - last) * (Step - along);
		smoothing_queue.push_back(last);
		along = 0.0f;
		if (smoothing_queue.size() == smoothing_kernel.size()) {
			Vector2f p = make_vector( 0.0f, 0.0f );
			for (unsigned int i = 0; i < smoothing_kernel.size(); ++i) {
				p += smoothing_kernel[i] * smoothing_queue[i];
			}
			Stroke::submit_point(p);
			smoothing_queue.pop_front();
		}
	}
}

