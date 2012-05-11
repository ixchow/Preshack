#include "Error.hpp"

std::ostream &get_error_stream() {
	return std::cerr;
}

std::ostream &get_warning_stream() {
	return std::cerr;
}

std::ostream &get_info_stream() {
	return std::cout;
}
