
#extension GL_ARB_texture_rectangle : enable

//f_small is approx 2x smaller than this.
// Assume: Texture coordinates aready set.
// (basically, this is to get around possible clamping if I use fixed-function)
uniform sampler2DRect f_small;
uniform sampler2DRect f_resid;
void main()
{
	gl_FragColor = texture2DRect(f_small, gl_TexCoord[0].xy)
	              +texture2DRect(f_resid, gl_TexCoord[1].xy);
}
