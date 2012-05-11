#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect f;
uniform sampler2DRect gy;

void main()
{
	gl_FragColor =
		texture2DRect(gy, gl_TexCoord[0].xy)
		- (texture2DRect(f, gl_TexCoord[0].xy+vec2(0,1))
		- texture2DRect(f, gl_TexCoord[0].xy));
}
