#ifndef LIST_GRAPH_HPP
#define LIST_GRAPH_HPP

#include <vector>

using std::vector;

class ListGraph {
public:
	vector< vector< unsigned int > > lists;
	vector< vector< unsigned int > > edges;
	void flip_rel(unsigned int list, unsigned int item, unsigned int rel, bool is_up);
	bool is_consistent();
	//test the fact 'a < b' at list.
	bool test_fact(unsigned int layers, vector< vector< bool > > &facts, unsigned int list, unsigned int a, unsigned int b);

	//assert() that facts are 'consistent'.
	void verify_facts(unsigned int layers, vector< vector< bool > > &facts);
};

//NOTE: reward[ i * tiebreak.size() + j ] => goodness of 'i' before 'j'
int set_order_greedy(vector< unsigned int > const &reward, vector< unsigned int > const &tiebreak, vector< unsigned int > &list);
//Return is the total score := rewards gotten - rewards not gotten; ~= how sure we are of this ordering.

#endif //LIST_GRAPH_HPP
