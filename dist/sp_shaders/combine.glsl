uniform sampler2D layerA;
uniform sampler2D layerB;
uniform sampler2D order;

void main()
{
	float amt = texture2D(order, gl_TexCoord[0].xy).x;
	vec4 ca = texture2D(layerA, gl_TexCoord[0].xy);
	vec4 cb = texture2D(layerB, gl_TexCoord[0].xy);

	if (ca.w + cb.w == 0.0) {
		gl_FragColor = vec4(0.0,0.0,0.0,0.0);
		return;
	}
	vec4 stack1;
	stack1.xyz = (ca.xyz * ca.w + cb.xyz * (1.0 - ca.w) * cb.w) / (ca.w + (1.0 - ca.w) * cb.w);
	stack1.w = ca.w + (1.0 - ca.w) * cb.w;
	vec4 stack2;
	stack2.xyz = (cb.xyz * cb.w + ca.xyz * (1.0 - cb.w) * ca.w) / (cb.w + (1.0 - cb.w) * ca.w);
	stack2.w = cb.w + (1.0 - cb.w) * ca.w;

	gl_FragColor = mix(stack1, stack2, amt);
}
