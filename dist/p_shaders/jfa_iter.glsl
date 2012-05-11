
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect close;
uniform float step;

#define TEST( X ) \
	{ \
		vec3 test = texture2DRect(close, gl_TexCoord[0].xy + (X)).xyz; \
		if (test.x != 0.0) { \
			test.z = dot(test.xy - pos, test.xy - pos); \
			if (test.z < best.z) { \
				best = test; \
			} \
		} \
	} \


void main() {

	vec2 pos = gl_TexCoord[0].xy;
	vec3 best = texture2DRect(close, gl_TexCoord[0].xy).xyz;
	TEST(vec2(0,step))
	TEST(vec2(0,-step))
	TEST(vec2(step,0))
	TEST(vec2(-step,0))
	TEST(vec2(step,step))
	TEST(vec2(-step,-step))
	TEST(vec2(step,-step))
	TEST(vec2(-step,step))

	gl_FragColor.xyz = best;
	gl_FragColor.w = 1.0;
}
