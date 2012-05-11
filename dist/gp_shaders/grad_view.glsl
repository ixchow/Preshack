
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect grad;

void main()
{
	vec4 g = texture2DRect(grad, gl_TexCoord[0].xy);

	gl_FragColor = g + 0.5;
}
