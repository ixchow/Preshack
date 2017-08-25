#include "PackedIO.hpp"

#include <vector>

void write_token(std::string val, std::ostream &out) {
	out.write(&(val[0]), val.size());
}
void write_packed(uint64_t value, std::ostream &out) {
	//write in 7-bit chunks w/ 'continue' bit:
	do {
		uint8_t c = (value & 0x7F);
		if (value > 0x7F) c = c | 0x80;
		out.write((char *)&c, 1);
		value = value >> 7;
	} while (value);
}
void write_packed(unsigned int value, std::ostream &out) {
	write_packed((uint64_t)value, out);
}
void write_packed(int64_t value, std::ostream &out) {
	//rotate sign bit to first bit in number:
	int64_t neg = (value < 0)?1:0;
	if (value >= 0) value = (value << 1) | neg;
	else value = (-value) << 1 | neg;
	write_packed((uint64_t)value, out);
}
void write_packed(int value, std::ostream &out) {
	write_packed((int64_t)value, out);
}
void write_packed(Vector2i value, std::ostream &out) {
	write_packed(value.c[0], out);
	write_packed(value.c[1], out);
}
void write_alpha(std::string const &value, std::ostream &out) {
	out.write(value.c_str(), value.size());
	char zero = '\0';
	out.write(&zero, 1);
}
void write_packed(float value, std::ostream &out) {
	out.write((char *)&(value), sizeof(float));
}
void write_packed(Vector2f value, std::ostream &out) {
	write_packed(value.x, out);
	write_packed(value.y, out);
}
void write_packed(Vector3f value, std::ostream &out) {
	write_packed(value.x, out);
	write_packed(value.y, out);
	write_packed(value.z, out);
}
void write_packed(Box2f value, std::ostream &out) {
	write_packed(value.min, out);
	write_packed(value.max, out);
}



bool read_token(std::string val, std::istream &in) {
	std::vector< char > temp(val.size());
	if (!(in.read(&(temp[0]), temp.size()))) return false;
	for (unsigned int i = 0; i < temp.size(); ++i) {
		if (temp[i] != val[i]) return false;
	}
	return true;
}
bool read_packed(uint64_t &value, std::istream &in) {
	//read in 7-bit chunks w/ 'continue' bit:
	value = 0;
	int shift = 0;
	while (1) {
		uint8_t c;
		if (!in.read((char *)&c, 1)) return false;
		value |= (uint64_t)(c & 0x7F) << shift;
		shift += 7;
		if (!(c & 0x80)) break;
	}
	return true;
}
bool read_packed(unsigned int &value, std::istream &in) {
	uint64_t temp;
	if (!read_packed(temp, in)) return false;
	value = temp;
	return true;
}
bool read_packed(int64_t &value, std::istream &in) {
	uint64_t temp;
	if (!read_packed(temp, in)) return false;
	value = (signed)(temp >> 1);
	if (temp & 1) value = -value;
	return true;
}
bool read_packed(int &value, std::istream &in) {
	int64_t temp;
	if (!read_packed(temp, in)) return false;
	value = temp;
	return true;
}
bool read_alpha(std::string &value, std::istream &in) {
	value = "";
	while (1) {
		char c;
		if (!in.read((char *)&c, 1)) return false;
		if (c == '\0') break;
		value += c;
	}
	return true;
}
bool read_packed(float &value, std::istream &in) {
	if (!in.read((char *)&value, sizeof(float))) return false;
	return true;
}
bool read_packed(Box2f &value, std::istream &in) {
	return read_packed(value.min, in) && read_packed(value.max, in);
}
