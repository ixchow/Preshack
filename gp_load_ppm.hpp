//Ok, just pasting the ppm loading stuff here because it's really geting over-duplicated:

#include <fstream>
using std::ifstream;
using std::ofstream;

bool load_ppm(string filename, unsigned int &width, unsigned int &height, vector< float > &data) {
	ifstream im(filename.c_str(), std::ios::in | std::ios::binary);
	char p, six;
	unsigned int maxval;
	if (!(im >> p >> six >> width >> height >> maxval)) {
		cerr << "Error reading '" << filename << "'" << endl;
		return false;
	}
	if (p != 'P' || six != '6') {
		cerr << "Error reading '" << filename << "': bad magic." << endl;
		return false;
	}
	if (width * height > 100000000) {
		cerr << "File '" << filename << "' fails size sanity cheack." << endl;
		return false;
	}
	if (maxval > 255) {
		cerr << "File '" << filename << "' has larger-than-byte maxval." << endl;
		return false;
	}
	char ws;
	if (!(im.read(&ws, 1))) {
		cerr << "Error reading single whitespace byte before raster." << endl;
		return false;
	}
	vector< unsigned char > pixdata(width*height*3);
	if (!im.read((char *)&(pixdata[0]), pixdata.size())) {
		cerr << "Error reading raster from file." << endl;
		return false;
	}
	data.resize(width*height*3);
	float inv = 1.0f / maxval;
	for (unsigned int i = 0; i < data.size(); ++i) {
		data[i] = pixdata[i] * inv;
	}
	return true;
}

bool load_ppm_channels(string filename, unsigned int &width, unsigned int &height, vector< float > &r, vector< float > &g, vector< float > &b) {
	vector< float > data;
	if (!load_ppm(filename, width, height, data)) {
		return false;
	}
	assert(width * height * 3 == data.size());
	r.resize(width * height);
	g.resize(width * height);
	b.resize(width * height);
	for (unsigned int i = 0; i < r.size(); ++i) {
		r[i] = data[i*3+0];
		g[i] = data[i*3+1];
		b[i] = data[i*3+2];
	}
	return true;
}

