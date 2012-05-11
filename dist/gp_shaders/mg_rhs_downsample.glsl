
#extension GL_ARB_texture_rectangle : enable

//Note: we're expecting texture coordinates to be 2x fragment coordinates.
uniform sampler2DRect rhs;
void main()
{
	gl_FragColor =
			 0.25*texture2DRect(rhs, gl_TexCoord[0].xy+vec2(-1,-1))
			+0.50*texture2DRect(rhs, gl_TexCoord[0].xy+vec2( 0,-1))
			+0.25*texture2DRect(rhs, gl_TexCoord[0].xy+vec2( 1,-1))
			+0.50*texture2DRect(rhs, gl_TexCoord[0].xy+vec2(-1, 0))
			+1.00*texture2DRect(rhs, gl_TexCoord[0].xy)
			+0.50*texture2DRect(rhs, gl_TexCoord[0].xy+vec2( 1, 0))
			+0.25*texture2DRect(rhs, gl_TexCoord[0].xy+vec2(-1, 1))
			+0.50*texture2DRect(rhs, gl_TexCoord[0].xy+vec2( 0, 1))
			+0.25*texture2DRect(rhs, gl_TexCoord[0].xy+vec2( 1, 1))
		;
}
