#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect img;
uniform sampler2D static_tex;
uniform vec2 static_mul;
uniform vec2 static_add;
uniform float scale;

void main()
{
	gl_FragColor =
		  texture2DRect(img, gl_TexCoord[0].xy)
		  + (texture2D(static_tex, gl_TexCoord[0].xy * static_mul + static_add) - 0.5) * scale;
	gl_FragColor.w = 1.0;
}
