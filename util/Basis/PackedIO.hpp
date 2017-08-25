#ifndef PACKED_IO_HPP
#define PACKED_IO_HPP

#include <Vector/Vector.hpp>
#include <Vector/Box.hpp>

#include <string>
#include <iostream>
#include <vector>

#include <stdint.h>

void write_token(std::string val, std::ostream &out);
void write_packed(uint64_t value, std::ostream &out);
void write_packed(unsigned int value, std::ostream &out);
void write_packed(int64_t value, std::ostream &out);
void write_packed(int value, std::ostream &out);
void write_packed(Vector2i value, std::ostream &out);
void write_alpha(std::string const &value, std::ostream &out);
void write_packed(float value, std::ostream &out);
void write_packed(Vector2f value, std::ostream &out);
void write_packed(Vector3f value, std::ostream &out);
void write_packed(Box2f value, std::ostream &out);

template< typename T, int SIZE >
void write_packed(Vector< T, SIZE > const &val, std::ostream &out) {
	for (unsigned int i = 0; i < SIZE; ++i) {
		write_packed(val.c[i], out);
	}
}

template< typename T >
void write_packed(std::vector< T > const &val, std::ostream &out) {
	write_packed((uint64_t)val.size(), out);
	for (typename std::vector< T >::const_iterator i = val.begin(); i != val.end(); ++i) {
		write_packed(*i, out);
	}
}

bool read_token(std::string val, std::istream &in); //checks that read token == 'val'
bool read_packed(uint64_t &value, std::istream &in);
bool read_packed(unsigned int &value, std::istream &in);
bool read_packed(int64_t &value, std::istream &in);
bool read_packed(int &value, std::istream &in);
bool read_alpha(std::string &value, std::istream &in);
bool read_packed(float &value, std::istream &in);
bool read_packed(Box2f &value, std::istream &in);

template< typename T, int SIZE >
bool read_packed(Vector< T, SIZE > &val, std::istream &in) {
	for (unsigned int i = 0; i < SIZE; ++i) {
		if (!read_packed(val.c[i], in)) return false;
	}
	return true;
}

template< typename T >
bool read_packed(std::vector< T > &val, std::istream &in) {
	uint64_t s;
	if (!read_packed(s, in)) return false;
	val.resize(s);
	for (typename std::vector< T >::iterator i = val.begin(); i != val.end(); ++i) {
		if (!read_packed(*i, in)) return false;
	}
	return true;
}

#endif //PACKED_IO_HPP
