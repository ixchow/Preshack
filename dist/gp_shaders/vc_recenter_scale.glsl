
#extension GL_ARB_texture_rectangle : enable

//Note: Just here to avoid possible color clampping. Seriously.
uniform sampler2DRect img;
void main()
{
	gl_FragColor = texture2DRect(img, gl_TexCoord[0].xy);
}
