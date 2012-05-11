#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect value; //w == distance to nearest constraint px
uniform sampler2DRect cons; //white where we should interpolate, black where we should ignore, color where we are constrained.

uniform float scale;

// Neighborhood:
//   s0
// s1  s2
//   s3

vec4 sample(vec2 at) {
	vec4 s = texture2DRect(cons, at);
	if (s.w == 0.0) {
		return texture2DRect(value, at);
	} else {
		return s;
	}
}

void main() {
	vec2 at = gl_TexCoord[0].xy;
	{ //check if we need to interpolate here:
		vec4 cur = texture2DRect(cons, at);
		if (cur.w != 0.0) {
			gl_FragColor = vec4(cur.xyz, 0.0);
			return;
		}
	}
	float dis = texture2DRect(value, at).w;
	float step = max(scale * dis, 1.0);

	vec4 sum =
		  sample(at + vec2( step,0))
		+ sample(at + vec2(-step,0))
		+ sample(at + vec2(0, step))
		+ sample(at + vec2(0,-step))
	;
	gl_FragColor = vec4(sum.xyz / 4.0, dis);
}
