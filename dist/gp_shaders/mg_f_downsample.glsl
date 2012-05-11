
#extension GL_ARB_texture_rectangle : enable

//Note: we're expecting texture coordinates to be 2x fragment coordinates.
uniform sampler2DRect f;
void main()
{
	gl_FragColor =
		0.125 * (
			 0.5*texture2DRect(f, gl_TexCoord[0].xy+vec2(-1,-1))
			+    texture2DRect(f, gl_TexCoord[0].xy+vec2( 0,-1))
			+0.5*texture2DRect(f, gl_TexCoord[0].xy+vec2( 1,-1))
			+    texture2DRect(f, gl_TexCoord[0].xy+vec2(-1, 0))
			+2.0*texture2DRect(f, gl_TexCoord[0].xy)
			+    texture2DRect(f, gl_TexCoord[0].xy+vec2( 1, 0))
			+0.5*texture2DRect(f, gl_TexCoord[0].xy+vec2(-1, 1))
			+    texture2DRect(f, gl_TexCoord[0].xy+vec2( 0, 1))
			+0.5*texture2DRect(f, gl_TexCoord[0].xy+vec2( 1, 1))
		)
		;
}
