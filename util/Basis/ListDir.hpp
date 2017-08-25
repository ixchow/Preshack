#ifndef LISTDIR_HPP
#define LISTDIR_HPP

#include <string>
#include <vector>

//returns all items in directory in no particular order -- note that
//isn't _just_ files..
bool list_dir(std::string dir, std::vector< std::string > &items, std::string suffix = "");

bool exists(std::string filename);
bool is_file(std::string filename);
bool is_directory(std::string filename);


#endif //LISTDIR_HPP
