
uniform sampler3D tags;
uniform sampler3D ball;
uniform sampler2D bars;
uniform sampler1D sel;
uniform sampler1D fade;

void main() {

	vec3 at = gl_TexCoord[0].xyz;
	float f = texture1D(fade, at.z).a;

	if (f == 0.0) {
		discard;
	}

	gl_FragColor = texture2D(bars, at.xy);
	gl_FragColor.a *= f;
}

