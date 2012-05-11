#extension GL_EXT_texture_array : enable
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect image;
uniform sampler2DRect coef;

#ifndef CHAN
#define CHAN a
#endif

void main() {
	vec4 i = texture2DRect(image, gl_TexCoord[0].xy);
	float a = texture2DRect(coef, gl_TexCoord[0].xy).CHAN;
	gl_FragColor = i  * a;
}
