#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect gx;
uniform sampler2DRect gy;

void main()
{
	gl_FragColor =
		0.25 * (
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-2,-1))
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-1,-1))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 0,-1))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 1,-1))
			//
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-2, 1))
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-1, 1))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 0, 1))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 1, 1))

			-texture2DRect(gy, gl_TexCoord[0].xy + vec2(-1,-2))
			-texture2DRect(gy, gl_TexCoord[0].xy + vec2(-1,-1))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2(-1, 0))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2(-1, 1))
			//
			-texture2DRect(gy, gl_TexCoord[0].xy + vec2( 1,-2))
			-texture2DRect(gy, gl_TexCoord[0].xy + vec2( 1,-1))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2( 1, 0))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2( 1, 1))
		)
		+0.5 * (
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-2, 0))
			-texture2DRect(gx, gl_TexCoord[0].xy + vec2(-1, 0))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 0, 0))
			+texture2DRect(gx, gl_TexCoord[0].xy + vec2( 1, 0))

			-texture2DRect(gy, gl_TexCoord[0].xy + vec2( 0,-2))
			-texture2DRect(gy, gl_TexCoord[0].xy + vec2( 0,-1))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2( 0, 0))
			+texture2DRect(gy, gl_TexCoord[0].xy + vec2( 0, 1))
		)
		;
}
