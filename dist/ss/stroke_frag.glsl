#extension GL_EXT_texture_array : enable
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect stroke;
uniform sampler2DRect from;

void main() {
	float s = texture2DRect(stroke, gl_TexCoord[0].xy).CHAN;
	float f = texture2DRect(from, gl_TexCoord[0].xy).CHAN;
	gl_FragColor = vec4(s * f, s * f, s * f, s * f);
}
