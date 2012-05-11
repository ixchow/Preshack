
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect image;
uniform float px_size;

// eight-queens / rotated grid:
//  . . . . . . x .
//  . x . . . . . .
//  . . . . x . . .
//  . . . . . . . x
//  . . x .'. . . .
//  . . . . . x . .
//  x . . . . . . .
//  . . . x . . . .

#define S0 vec2(-7.0/16.0, 5.0/16.0) * px_size
#define S1 vec2(-5.0/16.0,-5.0/16.0) * px_size
#define S2 vec2(-3.0/16.0, 1.0/16.0) * px_size
#define S3 vec2(-1.0/16.0, 7.0/16.0) * px_size
#define S4 vec2( 1.0/16.0,-3.0/16.0) * px_size
#define S5 vec2( 3.0/16.0, 3.0/16.0) * px_size
#define S6 vec2( 5.0/16.0,-7.0/16.0) * px_size
#define S7 vec2( 7.0/16.0,-1.0/16.0) * px_size

void main()
{
	vec2 at = gl_TexCoord[0].xy;
	vec4 c =
		0.125 * (
		   texture2DRect(image, at + S0)
		  +texture2DRect(image, at + S1)
		  +texture2DRect(image, at + S2)
		  +texture2DRect(image, at + S3)
		  +texture2DRect(image, at + S4)
		  +texture2DRect(image, at + S5)
		  +texture2DRect(image, at + S6)
		  +texture2DRect(image, at + S7)
		)
		;
	gl_FragColor = c;
}
