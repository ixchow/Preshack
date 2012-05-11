
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
	vec4 brush_tan = TEXTURE_LOOKUP(brush_gx_tex, gl_TexCoord[2].xy);
	vec4 brush_perp = TEXTURE_LOOKUP(brush_gy_tex, gl_TexCoord[3].xy);
	//Modulate by color:
	brush_tan *= gl_Color;
	brush_perp *= gl_Color;
	//Attenuate by alpha:
	brush_tan *= brush_tan.a;
	brush_perp *= brush_perp.a;

	vec4 brush_grad_x =
		dir.x * brush_tan
		+ -dir.y * brush_perp
	;
	vec4 brush_grad_y =
		dir.y * brush_tan
		+ dir.x * brush_perp
	;
	vec4 base_grad_x =
		TEXTURE_LOOKUP(base_gx_tex, gl_TexCoord[0].xy)
	;
	vec4 base_grad_y =
		TEXTURE_LOOKUP(base_gy_tex, gl_TexCoord[1].xy)
	;
	vec4 brush_len = brush_grad_x * brush_grad_x + brush_grad_y * brush_grad_y;
	vec4 base_len = base_grad_x * base_grad_x + base_grad_y * base_grad_y;

	vec4 pick_base = vec4(greaterThan(brush_len, base_len));
	vec4 pick_brush = vec4(lessThan(brush_len, base_len));

	gl_FragColor =
	#ifdef DO_X
		pick_base * base_grad_x + pick_brush * brush_grad_x
	#endif
	#ifdef DO_Y
		pick_base * base_grad_y + pick_brush * brush_grad_y
	#endif
	;
}