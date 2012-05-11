
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect gx_tex;
uniform sampler2DRect gy_tex;
uniform float fac;

void main()
{
	vec4 gx = texture2DRect(gx_tex, gl_TexCoord[0].xy);
	vec4 gy = texture2DRect(gy_tex, gl_TexCoord[0].xy);
	vec4 gxm = texture2DRect(gx_tex, gl_TexCoord[0].xy - vec2(1,0));
	vec4 gym = texture2DRect(gy_tex, gl_TexCoord[0].xy - vec2(0,1));
	gl_FragColor = ((gx - gxm) + (gy - gym)) * fac;
}
