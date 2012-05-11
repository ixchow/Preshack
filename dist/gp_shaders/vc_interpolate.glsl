
#extension GL_ARB_texture_rectangle : enable

//small is approx 2x smaller than this.
// Assume: Texture coordinates aready set.
// (basically, this is to get around possible clamping if I use fixed-function)
uniform sampler2DRect small;
void main()
{
	gl_FragColor = texture2DRect(small, gl_TexCoord[0].xy);
}
