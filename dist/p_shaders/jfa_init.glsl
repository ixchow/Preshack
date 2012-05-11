
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect cons;
uniform float size;

void main()
{
	vec4 c = texture2DRect(cons, gl_TexCoord[0].xy);
	if (c.w == 0.0) {
		gl_FragColor = vec4(0.0, 0.0, size*size, 1.0); //"closest seed is here with distance 'huge'"
	} else {
		gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0, 1.0); //"closest seed is here with distance 'small'"
	}
}
