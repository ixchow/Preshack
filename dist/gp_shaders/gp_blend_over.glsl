
#extension GL_ARB_texture_rectangle : enable

#ifdef ATI_HACK
vec4 fake_interp(sampler2DRect tex, vec2 pos) {
	pos = pos - vec2(0.5, 0.5);
	return mix(
		mix(texture2DRect(tex, vec2(floor(pos.x)+0.5, floor(pos.y)+0.5)),
			texture2DRect(tex, vec2(floor(pos.x)+0.5, ceil(pos.y)+0.5)),
			fract(pos.y)),
		mix(texture2DRect(tex, vec2(ceil(pos.x)+0.5, floor(pos.y)+0.5)),
			texture2DRect(tex, vec2(ceil(pos.x)+0.5, ceil(pos.y)+0.5)),
			fract(pos.y)),
		fract(pos.x));
}
#define TEXTURE_LOOKUP fake_interp
#else //ATI_HACK
#define TEXTURE_LOOKUP texture2DRect
#endif //ATI_HACK

//Expecting texture coordinates:
//0 -> base gx image.
//1 -> base gy image.
//2 -> gx image.
//3 -> gy image.
//4 -> direction
//gl_Color.rgb -> tint
//gl_Color.a -> overall alpha (used with gx/gy alpha?)

uniform sampler2DRect base_gx_tex;
uniform sampler2DRect base_gy_tex;

uniform sampler2DRect brush_gx_tex;
uniform sampler2DRect brush_gy_tex;

void main()
{
	vec2 dir = gl_TexCoord[4].xy;
	vec4 brush_grad_x = TEXTURE_LOOKUP(brush_gx_tex, gl_TexCoord[2].xy);
	vec4 brush_grad_y = TEXTURE_LOOKUP(brush_gy_tex, gl_TexCoord[3].xy);
	vec4 brush_grad =
#ifdef DO_X
		dir.x
#endif
#ifdef DO_Y
		dir.y
#endif
		* brush_grad_x * brush_grad_x.a
		+
#ifdef DO_X
		-dir.y
#endif
#ifdef DO_Y
		dir.x
#endif
		* brush_grad_y * brush_grad_y.a
	;
	gl_FragColor =
		mix(
#ifdef DO_X
		texture2DRect(base_gx_tex, gl_TexCoord[0].xy)
#endif
#ifdef DO_Y
		texture2DRect(base_gy_tex, gl_TexCoord[1].xy)
#endif
		,
			brush_grad * gl_Color, gl_Color.a
		);
}
