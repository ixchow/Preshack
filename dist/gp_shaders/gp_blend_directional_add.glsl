
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
	//Want to add:
	// (norm(base) * brush) * norm(base)
	// Try:
	//  (base * brush) * base / (base * base)

	vec4 len_sq = (base_grad_x * base_grad_x + base_grad_y * base_grad_y);
	len_sq += vec4(lessThan(len_sq, vec4(0.000001,0.000001,0.000001,0.000001))); //zero-len gets 1.
	vec4 amt = (brush_grad_y * base_grad_y + brush_grad_x * base_grad_x) / len_sq;
	//Protect from reversing edges:
	amt += vec4(1,1,1,1);
	amt *= vec4(greaterThan(amt, vec4(0,0,0,0)));
	amt -= vec4(1,1,1,1);
	gl_FragColor =
	#ifdef DO_X
		base_grad_x + base_grad_x
	#endif
	#ifdef DO_Y
		base_grad_y + base_grad_y
	#endif
		* amt
	;
}
