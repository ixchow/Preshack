
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect rhs;
void main()
{
	gl_FragColor =
		0.25 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3,-3))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2,-3))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1,-3))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0,-3))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1,-3))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2,-3))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3,-3))
		)
		+ 0.50 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3,-2))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2,-2))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1,-2))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0,-2))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1,-2))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2,-2))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3,-2))
		)
		+ 0.75 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3,-1))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2,-1))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1,-1))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0,-1))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1,-1))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2,-1))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3,-1))
		)
		+ 1.00 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3, 0))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2, 0))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1, 0))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0, 0))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1, 0))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2, 0))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3, 0))
		)
		+ 0.75 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3, 1))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2, 1))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1, 1))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0, 1))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1, 1))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2, 1))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3, 1))
		)
		+ 0.50 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3, 2))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2, 2))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1, 2))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0, 2))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1, 2))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2, 2))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3, 2))
		)
		+ 0.25 * (
			 0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-3, 3))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-2, 3))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2(-1, 3))
			+1.00 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 0, 3))
			+0.75 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 1, 3))
			+0.50 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 2, 3))
			+0.25 * texture2DRect(rhs, gl_TexCoord[0].xy + vec2( 3, 3))
		);


}
