
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect img;
uniform sampler2DRect sub;
uniform vec3 grey;

void main()
{
	gl_FragColor = texture2DRect(img, gl_TexCoord[0].xy)
	              -texture2DRect(sub, vec2(0.5,0.5))
				  +vec4(grey, 0.0);
}
