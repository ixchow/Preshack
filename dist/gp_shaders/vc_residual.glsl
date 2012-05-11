
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect f;
uniform sampler2DRect rhs;

//Description (rather terse, eh?) of the filter we're continuing:
uniform float w_corner;
uniform float w_edge;
uniform float w_center;

void main()
{
	gl_FragColor =
		texture2DRect(rhs, gl_TexCoord[0].xy)
		//filtered value:
		-   texture2DRect(f, gl_TexCoord[0].xy) * w_center
		- ( texture2DRect(f, gl_TexCoord[0].xy + vec2(1.0,0.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(-1.0,0.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(0.0,1.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(0.0,-1.0)) ) * w_edge
		
		- ( texture2DRect(f, gl_TexCoord[0].xy + vec2(1.0,1.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(1.0,-1.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(-1.0,1.0))
		  + texture2DRect(f, gl_TexCoord[0].xy + vec2(-1.0,-1.0)) ) * w_corner
		;
	gl_FragColor.a = 1.0;
}
