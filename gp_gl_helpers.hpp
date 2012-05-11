//some gl shorthand, because it was getting too duplicated.

namespace {

void set_clamp_to_black() {
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	Vector4f black = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
	glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, black.c);
}

void set_clamp_to_edge() {
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void set_nearest() {
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void set_linear() {
	#ifdef ATI_HACK
		assert(0);
	#endif
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void alloc_tex(unsigned int width, unsigned int height, GLuint &tex, vector< Vector3f > const &image = vector< Vector3f >(), bool alpha = false) {
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLenum int_fmt = alpha?GL_RGBA16F_ARB:GL_RGB16F_ARB;
	if (image.size() != width * height) {
		if (!image.empty()) {
			ERROR("Image passed to alloc_tex has wrong size.");
		}
		vector< Vector3f > blank(width * height, make_vector< float >(0,0,0));
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, int_fmt, width, height, 0, GL_RGB, GL_FLOAT, &(blank[0]));
	} else {
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, int_fmt, width, height, 0, GL_RGB, GL_FLOAT, &(image[0]));
		if (width > 10000 || height > 10000) {
			INFO("Alloc was " << width << "x" << height << " with format " << int_fmt);
			assert(0);
		}
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	Graphics::gl_errors("alloc_tex");
}

void alloc_fb(GLuint tex, GLuint &fb) {
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, tex, 0);

	GLenum ret = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	if (ret != GL_FRAMEBUFFER_COMPLETE_EXT) {
		WARNING("GX_FRAMEBUFFER NOT COMPLETE!");
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	Graphics::gl_errors("alloc_fb");
}

void bind_fb(GLuint framebuffer, unsigned int width, unsigned int height) {
	glPushAttrib(GL_VIEWPORT_BIT);

	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glTranslatef(-1.0f, -1.0f, 0.0f);
	glScalef(2.0f / width, 2.0f / height, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
}

void unbind_fb() {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glPopAttrib();
}

}
