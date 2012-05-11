
#version 110
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D src;
uniform sampler2DRect log_tex;
uniform sampler2D exp_tex;
uniform vec2 log_src;
uniform vec2 fac;
uniform float add_real;
uniform float add_imag;
uniform bool nomap;
uniform vec2 exp_fac;

void main()
{
	vec2 coord = gl_TexCoord[0].xy;
	if (!nomap) {
		coord = texture2DRect(log_tex, gl_TexCoord[0].xy).xy;
	}
	coord += vec2(add_real, add_imag);
	// -- coord gets transformed from dst to src space:

	//coord = vec2(coord.x * fac.x - coord.y * fac.y,
	//            coord.y * fac.x + coord.x * fac.y);

	// -- coord = exp(coord)
	// here we do some fading to match up over the scale breaks

	//translate back to the fundamental x range:
	coord = texture2D(exp_tex, vec2(coord.x * exp_fac.x, coord.y * exp_fac.y)).xy;
	//map into texture space:
	coord = coord * vec2(0.5,-0.5) + vec2(0.5, 0.5);

	gl_FragColor = texture2D(src, coord);
	gl_FragColor.a = gl_Color.a;
}
