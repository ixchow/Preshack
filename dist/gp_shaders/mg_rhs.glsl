
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect gx;
uniform sampler2DRect gy;
void main()
{
	gl_FragColor =
		(texture2DRect(gx, gl_TexCoord[0].xy)
		-texture2DRect(gx, gl_TexCoord[0].xy+vec2(-1,0)))
		+(texture2DRect(gy, gl_TexCoord[0].xy)
		-texture2DRect(gy, gl_TexCoord[0].xy+vec2(0,-1)))
		;
}
