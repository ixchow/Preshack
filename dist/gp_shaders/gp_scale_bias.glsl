
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect image;
uniform float scale;
uniform float bias;

void main()
{
	vec4 c = texture2DRect(image, gl_TexCoord[0].xy);
	gl_FragColor.rgb = c.rgb * scale + bias;
	gl_FragColor.a = 1.0;
}
