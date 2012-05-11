#include "LL_ListGraph.hpp"

#include <algorithm>
#include <iostream>

#include <cassert>

using std::swap;
using std::pair;
using std::make_pair;

#define LOG_ERROR( X ) std::cerr << X << std::endl
#define LOG_WARNING( X ) std::cout << X << std::endl
#define LOG_INFO( X ) std::cout << X << std::endl


void ListGraph::flip_rel(unsigned int list, unsigned int item, unsigned int rel, bool is_up) {
	assert(list < lists.size());
	assert(lists.size() == edges.size());
	assert(item != rel);
	while (1) {
		unsigned int item_ind = (unsigned)-1;
		unsigned int rel_ind = (unsigned)-1;
		for (unsigned int i = 0; i < lists[list].size(); ++i) {
			if (lists[list][i] == item) {
				assert(item_ind == (unsigned)-1);
				item_ind = i;
			}
			if (lists[list][i] == rel) {
				assert(rel_ind == (unsigned)-1);
				rel_ind = i;
			}
		}
		if (item_ind == (unsigned)-1) break;
		if (rel_ind == (unsigned)-1) break;
		assert(item_ind != rel_ind);
		assert(item_ind < lists[list].size());
		assert(rel_ind < lists[list].size());
		if ((item_ind > rel_ind) == is_up) break; //great, we're done.
		if (is_up) {
			swap(lists[list][item_ind], lists[list][item_ind+1]);
		} else {
			swap(lists[list][item_ind], lists[list][item_ind-1]);
		}
		unsigned int new_rel = lists[list][item_ind];
		for (unsigned int e = 0; e < edges[list].size(); ++e) {
			flip_rel(edges[list][e], item, new_rel, is_up);
		}
	}
}

bool ListGraph::is_consistent() {
	assert(lists.size() == edges.size());
	for (unsigned int l = 0; l < lists.size(); ++l) {
		for (unsigned int e = 0; e < edges[l].size(); ++e) {
			assert(edges[l][e] < lists.size());
			vector< unsigned int > const &A = lists[l];
			vector< unsigned int > const &B = lists[edges[l][e]];
			vector< unsigned int > positions;
			for (unsigned int i = 0; i < A.size(); ++i) {
				while (A[i] >= positions.size()) {
					positions.push_back((unsigned)-1);
				}
				assert(positions[A[i]] == (unsigned)-1);
				positions[A[i]]= i;
			}
			for (unsigned int i = 0; i + 1 < B.size(); ++i) {
				if (B[i] >= positions.size()) continue;
				if (B[i+1] >= positions.size()) continue;
				if (positions[B[i]] == (unsigned)-1) continue;
				if (positions[B[i+1]] == (unsigned)-1) continue;
				assert(positions[B[i]] != positions[B[i+1]]);
				if (positions[B[i]] > positions[B[i+1]]) {
					LOG_WARNING("Inconsistency between " << l << " and " << edges[l][e] << " on " << B[i] << " < " << B[i+1]);
					return false;
				}
			}
		}
	}
	return true;
}

bool ListGraph::test_fact(unsigned int layers, vector< vector< bool > > &facts, unsigned int list, unsigned int a, unsigned int b) {
	assert(facts.size() == lists.size());
	assert(list < lists.size());
	assert(facts[list].size() == layers * layers);
	assert(a < layers);
	assert(b < layers);
	assert(edges.size() == lists.size());

	//First off, can we even have this fact?
	bool have_a = false;
	bool have_b = false;
	for (unsigned int i = 0; i < lists[list].size(); ++i) {
		if (lists[list][i] == a) have_a = true;
		if (lists[list][i] == b) have_b = true;
	}
	if (!have_a) return true;
	if (!have_b) return true;

	if (facts[list][a * layers + b]) return true; //already know fact.
	if (facts[list][b * layers + a]) return false; //fact contradicts what we know.

	//ok, add the fact:
	facts[list][a * layers + b] = true;

	vector< pair< unsigned int, unsigned int > > added;
	added.push_back(make_pair(a,b));
	for (unsigned int i = 0; i < layers; ++i) {
		//anything < a is < b
		if (facts[list][i * layers + a]) {
			if (!facts[list][i * layers + b]) {
				if (facts[list][b * layers + i]) {
					assert(0); //deduced an incorrect fact?
					return false;
				}
				added.push_back(make_pair(i,b));
				facts[list][i * layers + b] = true;
			}
		}
		//anything > b is > a
		if (facts[list][b * layers + i]) {
			if (!facts[list][a * layers + i]) {
				if (facts[list][i * layers + a]) {
					assert(0); //deduced an incorrect fact?
					return false;
				}
				added.push_back(make_pair(a,i));
				facts[list][a * layers + i] = true;
			}
		}
		//any pair i < a, b < j -> i < j
		for (unsigned int j = 0; j < layers; ++j) {
			if (facts[list][i * layers + a] && facts[list][b * layers + j]) {
				if (!facts[list][i * layers + j]) {
					if (facts[list][j * layers + i]) {
						assert(0); //Should not be able to deduce a contra here.
						return false; //deduced a contradiction.
					}
					added.push_back(make_pair(i,j));
					facts[list][i * layers + j] = true;
				}
			}
		}
	}
	//DEBUG:
	/*for (unsigned int i = 0; i < added.size(); ++i) {
		LOG_INFO("Added " << added[i].first << " < " << added[i].second << " at " << list);
	}*/
	//recurse!
	for (unsigned int i = 0; i < added.size(); ++i) {
		for (unsigned int e = 0; e < edges[list].size(); ++e) {
			bool good = test_fact(layers, facts, edges[list][e], added[i].first, added[i].second);
			if (!good) return false;
		}
	}
	return true;
}

void ListGraph::verify_facts(unsigned int layers, vector< vector< bool > > &facts) {
	assert(lists.size() == edges.size());
	assert(facts.size() == lists.size());
	for (unsigned int l = 0; l < lists.size(); ++l) {
		vector< bool > have(layers, false);
		vector< unsigned int > &list = lists[l];
		for (unsigned int i = 0; i < list.size(); ++i) {
			have[list[i]] = true;
		}
		vector< bool > const &facts1 = facts[l];
		assert(facts1.size() == layers * layers);
		for (unsigned int e = 0; e < edges[l].size(); ++e) {
			vector< unsigned int > &list2 = lists[edges[l][e]];
			vector< bool > const &facts2 = facts[edges[l][e]];
			assert(facts2.size() == layers * layers);
			for (unsigned int i = 0; i < list2.size(); ++i) {
				if (!have[list2[i]]) continue;
				for (unsigned int j = i+1; j < list2.size(); ++j) {
					if (!have[list2[j]]) continue;
					unsigned int a = list2[i];
					unsigned int b = list2[j];
					if (facts1[a * layers + b] != facts2[a * layers + b]
						|| facts1[b * layers + a] != facts2[b * layers + a]) {
						LOG_ERROR("Fact-mis-match between " << l << " and " << edges[l][e] << " on " << a << " < " << b);
						assert(0);
					}
				}
			}
		}
	}
}


int set_order_greedy(vector< unsigned int > const &reward, vector< unsigned int > const &tiebreak, vector< unsigned int > &list) {
	assert(reward.size() == tiebreak.size() * tiebreak.size());
	vector< unsigned int > to_order = list;
	list.clear();
	int total_score = 0;
	while (!to_order.empty()) {
		int best_score = 0;
		unsigned int best_ind = (unsigned)-1;
		for (unsigned int i = 0; i < to_order.size(); ++i) {
			int score = 0;
			for (unsigned int j = 0; j < to_order.size(); ++j) {
				assert(to_order[i] < tiebreak.size());
				assert(to_order[j] < tiebreak.size());
				score += reward[to_order[i] * tiebreak.size() + to_order[j]];
				score -= (int)reward[to_order[j] * tiebreak.size() + to_order[i]];
			}
			if (i == 0 || best_score < score) {
				best_ind = i;
				best_score = score;
			} else if (best_score == score) {
				for (unsigned int t = 0; t < tiebreak.size(); ++t) {
					if (tiebreak[t] == to_order[best_ind]) {
						break;
					}
					if (tiebreak[t] == to_order[i]) {
						best_ind = i;
						best_score = score;
						break;
					}
				}
			}
		}
		assert(best_ind < to_order.size());
		list.push_back(to_order[best_ind]);
		to_order.erase(to_order.begin() + best_ind);
		total_score += best_score;
	}
	return total_score;
}
