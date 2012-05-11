#include "ListDir.hpp"

#include <Basis/Error.hpp>

#ifdef WINDOWS
//windows-y directory listing.
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
//linux-specific:
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

bool list_dir(std::string dirname, std::vector< std::string > &items, std::string suffix) {
#ifndef WINDOWS
	DIR *dir = opendir(dirname.c_str());
	if (dir == NULL) {
		LOG_WARNING("Cannot open directory '" << dirname << "'");
		return false;
	} else {
		struct dirent *ent = NULL;
		while ((ent = readdir(dir))) {
			std::string name = std::string(ent->d_name);
#else
	struct _finddata_t fileinfo;
	intptr_t handle = _findfirst((dirname + "\\*").c_str(), &fileinfo);
	if (handle == -1) {
		LOG_WARNING("Cannot _findfirst on '" << dirname << "'");
		return false;
	} else {
		while (1) {
			std::string name = std::string(fileinfo.name);
#endif

		if (name.size() >= suffix.size() && name.substr(name.size()-suffix.size(), suffix.size()) == suffix) {
			items.push_back(name);
		}

#ifndef WINDOWS
		}
#else
			if (0 != _findnext(handle, &fileinfo)) break;
		}
		_findclose(handle);
#endif
	}
	return true;
}

#ifndef WINDOWS

bool exists(std::string filename) {
	struct stat info;
	if (stat(filename.c_str(), &info) == 0) {
		return true;
	} else {
		return false;
	}
}

bool is_file(std::string filename) {
	struct stat info;
	stat(filename.c_str(), &info);
	return(S_ISREG(info.st_mode));
}

bool is_directory(std::string filename) {
	struct stat info;
	stat(filename.c_str(), &info);
	return(S_ISDIR(info.st_mode));
}
#else //!!WINDOWS

bool exists(std::string filename) {
	struct _stat info;
	return(_stat(filename.c_str(), &info) == 0);
}

bool is_file(std::string filename) {
	struct _stat info;
	_stat(filename.c_str(), &info);
	return((info.st_mode & _S_IFREG) != 0);
}

bool is_directory(std::string filename) {
	struct _stat info;
	_stat(filename.c_str(), &info);
	return((info.st_mode & _S_IFDIR) != 0);
}


#endif //!WINDOWS
