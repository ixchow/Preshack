
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

	float r = texture3D(tags, at).x;
	vec4 bal = texture3D(ball, at);
	vec4 bar = texture2D(bars, at.xy);


	if (texture1D(sel,r).r > 0.5) {
		gl_FragColor = mix(bar,bal,bal.a);
	} else {
		gl_FragColor = mix(bal,bar,bar.a);
	}
	gl_FragColor.a *= f;
}

