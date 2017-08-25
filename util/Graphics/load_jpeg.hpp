#ifndef LOAD_JPEG_HPP
#define LOAD_JPEG_HPP

#include <string>
#include <vector>
#include <stdint.h>

bool load_jpeg(std::string filename, unsigned int &width, unsigned int &height, std::vector< uint32_t > &data);

#endif //LOAD_JPEG_HPP
