#ifndef LOAD_SAVE_PNG_HPP
#define LOAD_SAVE_PNG_HPP

#include <string>
#include <vector>
#include <stdint.h>

/*
 * Load and save PNG files.
 * NOTE: these functions perform flipping to place keep the
 *       pixel origin in the LOWER LEFT!
 *        (unless you specify otherwise by setting flip to false)
 */

bool load_png(std::string filename, unsigned int &width, unsigned int &height, std::vector< uint32_t > &data, bool flip = true);
void save_png(std::string filename, unsigned int width, unsigned int height, uint32_t const *data);

bool load_png(std::istream &from, unsigned int &width, unsigned int &height, std::vector< uint32_t > &data, bool flip = true);
void save_png(std::ostream &to, unsigned int width, unsigned int height, uint32_t const *data);


#endif
