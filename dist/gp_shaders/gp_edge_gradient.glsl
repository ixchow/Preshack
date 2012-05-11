#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect edge;
//calculates whatever gradient you want, you just have to set up the texcoords.
void main()
{
	gl_FragColor =
			( //gradient in edge:
				 texture2DRect(edge, gl_TexCoord[0].xy)
				-texture2DRect(edge, gl_TexCoord[1].xy)
			)
		;
	gl_FragColor.a = 1.0;
}
