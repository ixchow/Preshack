
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect f;
uniform sampler2DRect rhs;

//Description (rather terse, eh?) of the filter we're trying to invert:
uniform float w_corner;
uniform float w_edge;
uniform float w_center_inv;

void main()
{
	gl_FragColor = //vec4(mod(gl_TexCoord[0].x * 0.4,1.0), mod(gl_TexCoord[0].y*0.1,1.0)-0.5, 0.0, 1.0);
		//old value:
		  0.1 * texture2DRect(f, gl_TexCoord[0].xy)
		//guessed value:
		  + 0.9 * w_center_inv * (
		- ( texture2DRect(f, gl_TexCoord[0].xy + vec2(1.0,0.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(-1.0,0.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(0.0,1.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(0.0,-1.0)) ) * w_edge
		+ texture2DRect(rhs, gl_TexCoord[0].xy));
	gl_FragColor.w = 1.0;
}
