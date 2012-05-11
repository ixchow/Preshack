#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect img;
uniform sampler2DRect orig;
uniform float scale;
uniform float bias;
//expecting texture coordinates to be fragment posn.
void main()
{
	gl_FragColor =
			abs(
				texture2DRect(img, gl_TexCoord[0].xy)
				-texture2DRect(orig, gl_TexCoord[0].xy)
			) * scale
		;
	gl_FragColor.a = 1.0;
}
