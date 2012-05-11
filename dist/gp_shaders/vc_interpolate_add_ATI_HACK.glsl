
#extension GL_ARB_texture_rectangle : enable

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

//small is approx 2x smaller than this.
// Assume: Texture coordinates aready set.
// (basically, this is to get around possible clamping if I use fixed-function)
uniform sampler2DRect small;
uniform sampler2DRect add;
void main()
{
	gl_FragColor = fake_interp(small, gl_TexCoord[0].xy)
	              +texture2DRect(add, gl_TexCoord[1].xy);
}
