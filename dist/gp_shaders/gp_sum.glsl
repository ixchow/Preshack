
#extension GL_ARB_texture_rectangle : enable

//Expecting texture coordinates[0] to be for tex,
//coord[1] for base image
//produces (alpha-blended?) sum.
uniform sampler2DRect tex;
uniform sampler2DRect base; //base image
void main()
{
	vec4 tex_col = texture2DRect(tex, gl_TexCoord[0].xy);
	gl_FragColor =
			texture2DRect(base, gl_TexCoord[1].xy)
			+ tex_col.a * gl_Color.a * gl_Color * tex_col
		;
}
