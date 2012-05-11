#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect tags;
uniform vec4 highlight;
uniform vec3 highlight_color;
uniform float size;
uniform float px_size;

#define S0 vec2(-1.0/10.0,-3.0/10.0) * px_size
#define S1 vec2(-3.0/10.0, 1.0/10.0) * px_size
#define S2 vec2( 1.0/10.0, 3.0/10.0) * px_size
#define S3 vec2( 3.0/10.0,-1.0/10.0) * px_size

#define SAMP( L ) \
	{ \
		vec2 at = gl_TexCoord[0].xy + L; \
		vec4 tag1 = texture2DRect(tags, at + vec2(-size,-size)); \
		vec4 tag2 = texture2DRect(tags, at + vec2( size, size)); \
		vec4 tag3 = texture2DRect(tags, at + vec2(-size, size)); \
		vec4 tag4 = texture2DRect(tags, at + vec2( size,-size)); \
		 \
		if (tag1 != tag2 || tag3 != tag4) { \
			acc += vec3(0.0, 0.0, 0.0); \
		} else if (tag1 == highlight) { \
			acc += highlight_color; \
		} else { \
			acc += vec3(1.0, 1.0, 1.0); \
		} \
	}

void main() {
	vec3 acc = vec3(0.0, 0.0, 0.0);
	SAMP( S0 );
	SAMP( S1 );
	SAMP( S2 );
	SAMP( S3 );
	gl_FragColor.rgb = acc * 0.25;
	gl_FragColor.a = 1.0;
}

