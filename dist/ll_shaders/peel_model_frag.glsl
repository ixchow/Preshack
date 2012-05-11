#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect max_depth;

void main() {
	float depth = texture2DRect(max_depth, gl_FragCoord.xy).x;
	gl_FragColor = gl_Color;
	if (gl_FragCoord.z <= depth) {
		discard;
	}
}
