
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect close;
uniform sampler2DRect cons;

void main()
{
	vec2 pos = gl_TexCoord[0].xy;
	vec3 close = texture2DRect(close, pos).xyz;
	if (close.x == 0.0) {
		gl_FragColor = vec4(0.0, 0.0, 1.0, 0.0);
	} else {
		vec4 c = texture2DRect(cons, close.xy);
		gl_FragColor = vec4(c.xyz,
			sqrt(dot(close.xy - pos, close.xy - pos)));
	}
	/* //DEBUG: visualize distances:
	gl_FragColor.x = 0.0; //sqrt(dot(close.xy - pos, close.xy - pos)) / 600.0;
	gl_FragColor.yz = close.xy / 600.0;
	gl_FragColor.w = 1.0;
	*/
	
}
