#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect f;
uniform sampler2DRect gx;

void main()
{
	gl_FragColor =
		texture2DRect(gx, gl_TexCoord[0].xy)
		- (texture2DRect(f, gl_TexCoord[0].xy+vec2(1,0))
		- texture2DRect(f, gl_TexCoord[0].xy));
}
