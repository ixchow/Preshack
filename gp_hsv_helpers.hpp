#ifndef HSV_HPP
#define HSV_HPP

namespace {

//as per http://www.cs.rit.edu/~ncs/color/t_convert.html
Vector3f rgb_to_hsv( Vector3f rgb ) {
	float r = rgb.x;
	float g = rgb.y;
	float b = rgb.z;

	Vector3f ret = make_vector(0.0f, 0.0f, 0.0f);
	float *h = &ret.x;
	float *s = &ret.y;
	float *v = &ret.z;

	float min, max, delta;

	min = r;
	if (g < min) min = g;
	if (b < min) min = b;
	max = r;
	if (g > max) max = g;
	if (b > max) max = b;
	*v = max;				// v

	delta = max - min;

	if( max != 0.0f )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = 0.0f;
		return ret;
	}

	if (delta != 0.0f) {
		if( r == max )
			*h = ( g - b ) / delta;		// between yellow & magenta
		else if( g == max )
			*h = 2 + ( b - r ) / delta;	// between cyan & yellow
		else
			*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	}

	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
	*h /= 360.0f;
	return ret;
}

Vector3f hsv_to_rgb( Vector3f hsv ) {

	float h = hsv.x * 360.0f;
	if (h > 360.0f) h -= 360.0f;
	if (h < 0.0f) h += 360.0f;
	float s = hsv.y;
	float v = hsv.z;

	Vector3f ret = make_vector(0.0f, 0.0f, 0.0f);
	float *r = &ret.x;
	float *g = &ret.y;
	float *b = &ret.z;

	int i;
	float f, p, q, t;

	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return ret;
	}

	h /= 60;			// sector 0 to 5
	i = (int)h;
	f = h - i;			// factorial part of h
	if (i == 6) { //hack around h == 1 case not seeming to work right.
		i = 5; f = 1.0f;
	}
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
	return ret;
}

}

#endif //HSV_HPP
