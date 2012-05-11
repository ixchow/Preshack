#ifndef CACHE_HPP
#define CACHE_HPP

//#include "Data.hpp"

#include <vector>
#include <string>

using std::vector;
using std::string;
using std::pair;
using std::make_pair;

namespace Cache {
	enum {
		DEPENDS_INPUT_FRAGMENTS           = 1 << 0,
		DEPENDS_INPUT_BINS                = 1 << 1,
		//DEPENDS_INPUT_TRACES              = 1 << 2,
		DEPENDS_INPUT_DELTA               = 1 << 3,
		DEPENDS_INPUT_DISCOUNT            = 1 << 4,
		DEPENDS_INPUT_CONTROL_QUALITY_WEIGHT = 1 << 5,
		//DEPENDS_SETUP_TRANSITION_QUALITY  = 1 << 5,
		//DEPENDS_SETUP_CONTROL_QUALITY     = 1 << 6,
		//DEPENDS_SETUP_CONTROL_MODEL       = 1 << 7,
		//DEPENDS_POLICY_POLICY             = 1 << 8,
		//DEPENDS_POLICY_EXPECTED_REWARD    = 1 << 9,
		//DEPENDS_EVALUATION_OPT_ANIMATIONS = 1 << 10,
		//DEPENDS_EVALUATION_OPT_QUALITIES  = 1 << 11,
		DEPENDS_LIBRARY                   = 1 << 31,
	};

	typedef pair< void *, unsigned int > DataRange;
	typedef vector< DataRange > DataVec;

	//convenience class:
	class CacheFile {
	public:
		CacheFile( string name, unsigned int depends );
		bool load( DataVec const &data );
		void save( DataVec const &data );
	private:
		string filename;
	};


	template< typename TYPE >
	inline DataVec to_data(TYPE const &t);

	template< typename TYPE >
	inline DataVec to_data(vector< TYPE > const &t) {
		DataVec ret;
		for (unsigned int i = 0; i < t.size(); ++i) {
			DataVec add = to_data(t[i]);
			ret.insert(ret.end(), add.begin(), add.end());
		}
		return ret;
	}

	template< typename TYPE, typename TYPE2 >
	inline DataVec to_data(TYPE const &t, TYPE2 const &t2) {
		DataVec ret1 = to_data(t);
		DataVec ret2 = to_data(t2);
		ret1.insert(ret1.end(), ret2.begin(), ret2.end());
		return ret1;
	}

	#define BASIC_DEF( X ) \
	template< > \
	inline DataVec to_data(vector< X > const &v) { \
		DataVec ret; \
		ret.push_back(make_pair((void *)&(v[0]), v.size() * sizeof( X ))); \
		return ret; \
	}

	BASIC_DEF( float )
	BASIC_DEF( double )
	BASIC_DEF( int )
	BASIC_DEF( unsigned int )
}

#endif //CACHE_HPP
