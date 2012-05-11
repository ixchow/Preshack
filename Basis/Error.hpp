#ifndef ERROR_HPP
#define ERROR_HPP

#include <iostream>

std::ostream &get_error_stream();
//Old, deprecated:
#ifdef COMPAT_ERROR
#undef ERROR
#define ERROR( X ) get_error_stream() << "(deprecated error log fn)" << __FILE__ << __LINE__ << X << std::endl
#endif
#undef LOG_ERROR
#define LOG_ERROR( X ) get_error_stream() << X << std::endl

std::ostream &get_warning_stream();
#ifdef COMPAT_ERROR
#undef WARNING
#define WARNING( X ) get_warning_stream() << "(old warning fn)" << __FILE__ << __LINE__ << X << std::endl
#endif
#undef LOG_WARNING
#define LOG_WARNING( X ) get_warning_stream() << X << std::endl

std::ostream &get_info_stream();
#ifdef COMPAT_ERROR
#undef INFO
#define INFO( X ) get_info_stream() << "(old info fn)" << __FILE__ << __LINE__ << X << std::endl
#endif
#undef LOG_INFO
#define LOG_INFO( X ) get_info_stream() << X << std::endl

#endif //ERROR_HPP
