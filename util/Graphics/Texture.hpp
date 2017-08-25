#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

#include "Graphics.hpp"

#include "Cache.hpp"

#include <string>
#include <vector>

namespace Graphics {

using std::string;
using std::vector;

const unsigned int TEXTURE_ID = 0xabadbad1;

typedef bool (*ProcessFn)(unsigned int *tw, unsigned int *th, uint8_t **data, GLuint *internal_format, GLuint *data_format);

class CachedTexture : public CachableItem {
public:
	CachedTexture(string const &filename, bool pad, bool flip, ProcessFn process);
	virtual ~CachedTexture();
	virtual void reload();
	string filename;
	bool pad, flip;
	GLuint obj;
	unsigned int w,h;
	unsigned int tw, th; //if pad == true, then texture has been padded to a power of two and is really tw x th.
	ProcessFn process;
};

typedef CacheRef< CachedTexture > TextureRef;

//One of these is the function you (Mme. User) need to call:
TextureRef get_texture(string const &filename, bool pad = false, bool flip = true, ProcessFn process = NULL);
inline TextureRef get_texture(string const &filename, ProcessFn process) {
	return get_texture(filename, false, false, process);
}

} //namespace Graphics

#endif //GRAPHICS_TEXTURE_HPP
