#include "Cache.hpp"
#include "GameStuff.hpp"

#include <Library/Library.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

using std::cout;
using std::endl;

using std::ifstream;
using std::ofstream;

using std::ostringstream;

namespace Cache {

namespace {
	
	//from: http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
	const unsigned long long HashPrime = 1610612741ULL;

	void hash_range(unsigned int &out, const unsigned char *begin, unsigned int size) {
		unsigned long long hash_temp = out;
		for (unsigned int s = 0; s < size; ++s) {
			hash_temp = (hash_temp * 256 + (unsigned long long)begin[s]) % HashPrime;
		}
		out = hash_temp;
	}

	template< typename TYPE >
	void hash(unsigned int &out, TYPE const &val);

	template< typename TYPE >
	void hash(unsigned int &out, TYPE *foo) {
		hash(out, *foo);
	}

	template< typename TYPE >
	void hash(unsigned int &out, vector< TYPE > const &vec) {
		for (unsigned int e = 0; e < vec.size(); ++e) {
			hash(out, vec[e]);
		}
	}

	#define QUICK_HASH( X ) \
	template< > \
	void hash(unsigned int &out, vector< X > const &vec) { \
		hash_range(out, (unsigned char *)&(vec[0]), vec.size() * sizeof( X )); \
	}

	QUICK_HASH( float )
	QUICK_HASH( unsigned int )
	QUICK_HASH( int )
	//QUICK_HASH( Trace::TracePoint )
	//QUICK_HASH( Game2d::Control )
	QUICK_HASH( Game2d::Fragment )

	#define BASIC_HASH( X ) \
	template< > \
	void hash(unsigned int &out, X const &x) { \
		hash_range(out, (unsigned char *)&(x), sizeof( X )); \
	}

	BASIC_HASH( float )
	BASIC_HASH( int )
	BASIC_HASH( unsigned int )
	BASIC_HASH( Game2d::Control )

	/*template< >
	void hash(unsigned int &out, Trace const &foo) {
		hash(out, foo.trace);
	}*/

	string get_filename(string name, unsigned int depends) {
		ostringstream ret;
		ret << "cache/" << name;
		unsigned int hash_val = 0;
		if (depends & DEPENDS_INPUT_FRAGMENTS) {
			depends ^= DEPENDS_INPUT_FRAGMENTS;
			hash(hash_val, Game2d::fragments);
			cout << "After fragments: " << hash_val << endl; //DEBUG
		}

		if (depends & DEPENDS_INPUT_BINS) {
			depends ^= DEPENDS_INPUT_BINS;
			hash(hash_val, Game2d::bins);
			cout << "After bins: " << hash_val << endl; //DEBUG
		}

		/*if (depends & DEPENDS_INPUT_TRACES) {
			hash(hash_val, Input::traces);
		}*/

		if (depends & DEPENDS_INPUT_DELTA) {
			depends ^= DEPENDS_INPUT_DELTA;
			hash(hash_val, Game2d::Delta);
		}

		if (depends & DEPENDS_INPUT_DISCOUNT) {
			depends ^= DEPENDS_INPUT_DISCOUNT;
			hash(hash_val, Game2d::Discount);
		}

		if (depends & DEPENDS_INPUT_CONTROL_QUALITY_WEIGHT) {
			depends ^= DEPENDS_INPUT_CONTROL_QUALITY_WEIGHT;
			hash(hash_val, Game2d::ControlQualityWeight);
		}

		if (depends & DEPENDS_LIBRARY) {
			depends ^= DEPENDS_LIBRARY;
			hash(hash_val, Library::signature);
		}

		assert(depends == 0);

		ret << "." << hash_val << ".cache";

		return ret.str();
	}
	const unsigned int TokenSize = 6;
	char Token[TokenSize] = {'C', 'a', '$', 'h', 'e', 'd'};
}

CacheFile::CacheFile( string name, unsigned int depends ) {
	filename = get_filename( name, depends );
}

bool CacheFile::load( vector< pair< void *, unsigned int > > const &data ) {
	ifstream file(filename.c_str());
	if (!file) return false;
	for (unsigned int w = 0; w < data.size(); ++w) {
		if (!file.read((char *)data[w].first, data[w].second)) return false;
	}
	char token_buffer[TokenSize];
	if (!file.read(token_buffer, TokenSize)) return false;
	for (unsigned int i = 0; i < TokenSize; ++i) {
		if (token_buffer[i] != Token[i]) {
			return false;
		}
	}
	return true;
}

void CacheFile::save( vector< pair< void *, unsigned int > > const &data ) {
	ofstream file(filename.c_str());
	for (unsigned int w = 0; w < data.size(); ++w) {
		file.write((char *)data[w].first, data[w].second);
	}
	file.write(Token, TokenSize);
}


}
