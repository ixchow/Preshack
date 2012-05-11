#extension GL_EXT_texture_array : enable
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D brush;

void main() {
	float b = texture2D(brush, gl_TexCoord[0].xy).a;
	if (b < 0.5) {
		discard;
	} else {
		gl_FragColor = gl_Color;
	}
}
