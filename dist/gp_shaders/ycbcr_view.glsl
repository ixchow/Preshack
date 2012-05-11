
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect image;
//uniform float Kb;
//uniform float Kr;
uniform vec4 rfactor;
uniform vec4 gfactor;
uniform vec4 bfactor;

void main()
{
	vec4 c = texture2DRect(image, gl_TexCoord[0].xy);
	//c.x == Y, c.y == Cb, c.z == Cr

	//vec4 o;
	//o.a = 1.0;
	//o.b = c.y * 2.0 * (1.0 - Kb) + c.x;
	//o.r = c.z * 2.0 * (1.0 - Kr) + c.x;
	//o.g = (c.x - Kr * o.r + Kb * o.b) / (1.0 - Kr - Kb);
	gl_FragColor.r = dot(rfactor.xyz, c.xyz) + rfactor.w;
	gl_FragColor.g = dot(gfactor.xyz, c.xyz) + gfactor.w;
	gl_FragColor.b = dot(bfactor.xyz, c.xyz) + bfactor.w;
	gl_FragColor.a = 1.0;
}
