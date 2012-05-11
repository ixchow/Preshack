
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect image;
uniform float scale;
uniform float bias;

#define S0 (0.5*0.125)
#define S1 (1.5*0.125)
#define S2 (2.5*0.125)
#define S3 (3.5*0.125)
#define S4 (4.5*0.125)
#define S5 (5.5*0.125)
#define S6 (6.5*0.125)
#define S7 (7.5*0.125)

void main()
{
	vec2 min_px = gl_TexCoord[0].xy;
	vec2 max_px = gl_TexCoord[1].xy;
	vec4 c =
		0.125 * (
		  texture2DRect(image, mix(min_px, max_px, vec2(S0,S6)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S1,S1)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S2,S4)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S3,S7)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S4,S2)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S5,S5)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S6,S0)))
		  +texture2DRect(image, mix(min_px, max_px, vec2(S7,S3)))
		)
		;
	gl_FragColor.rgb = c.rgb * scale + bias;
	gl_FragColor.a = 1.0;
}
