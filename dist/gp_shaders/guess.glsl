
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect guess;
uniform sampler2DRect grad_x;
uniform sampler2DRect grad_y;

void main()
{
	vec4 u = texture2DRect(guess, gl_FragCoord.xy + vec2(0, 1));
	vec4 d = texture2DRect(guess, gl_FragCoord.xy + vec2(0,-1));
	vec4 g_up = texture2DRect(grad_y, gl_FragCoord.xy + vec2(0,1));
	vec4 g_pd = texture2DRect(grad_y, gl_FragCoord.xy);

	vec4 l = texture2DRect(guess, gl_FragCoord.xy + vec2( 1,0));
	vec4 r = texture2DRect(guess, gl_FragCoord.xy + vec2(-1,0));
	vec4 g_lp = texture2DRect(grad_x, gl_FragCoord.xy + vec2(1,0));
	vec4 g_pr = texture2DRect(grad_x, gl_FragCoord.xy);

	vec4 p = texture2DRect(guess, gl_FragCoord.xy);

	//vec3 grad = 2 * ( p - l + g_lp + p - r - g_pr + p - u + g_up + p - d - g_pd );
	//p = 0.25 * (l + r + u + d - g_lp + g_pr - g_up + g_pd);
	p = 0.25 * (l + r + u + d - g_lp + g_pr - g_up + g_pd);
	p.w = 1.0;

	gl_FragColor = p;
}
