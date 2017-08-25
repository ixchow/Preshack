#include <Graphics/load_save_png.hpp>
#include <Basis/Error.hpp>
#include <Vector/Vector.hpp>
#include "SS_optimizer.hpp"
#include "SS_coefs.hpp"
#include "SS_nice_dump.hpp"

#include <SDL_thread.h>

#include <mkl_blas.h>
#include <mkl_dss.h>

#include <sys/time.h>

#include <vector>
#include <string>
#include <sstream>
#include <set>

using std::vector;
using std::string;
using std::set;
using std::pair;
using std::make_pair;

class Timer {
public:
	Timer() {
		time_point("(init)");
	}
	~Timer() {
		time_point("(end)");
		std::cout << "\"event\",\"sec\"\n";
		for (unsigned int p = 0; p < points.size(); ++p) {
			assert(points[p].first.tv_usec < 1000000);
			double elapsed;
			elapsed = double(points[p].first.tv_sec) - double(points[0].first.tv_sec);
			elapsed += (double(points[p].first.tv_usec) - double(points[0].first.tv_usec)) / 1000000.0f;
			std::cout << '"' << points[p].second << "\", " << elapsed << "\n";
		}
		std::cout.flush();
	}
	void time_point(std::string name) {
		struct timeval now;
		gettimeofday(&now, NULL);
		points.push_back(make_pair(now, name));
		if (points.size() > 1) {
			double elapsed;
			elapsed = double(points.back().first.tv_sec) - double(points[0].first.tv_sec);
			elapsed += (double(points.back().first.tv_usec) - double(points[0].first.tv_usec)) / 1000000.0f;
			std::cout << "Elapsed at \"" << points.back().second << "\" is " << elapsed << std::endl;
		}

	}
	void time_point(std::string name, Vector2ui size) {
		std::ostringstream n;
		n << name << "-" << size.x << "x" << size.y;
		time_point(n.str());
	}
	vector< pair< struct timeval, std::string > > points;
};


void Layer::perm_from_alpha(Vector2ui size) {
	assert(size.x * size.y == data.size());
	perm.resize(size.y * (size.x-1 + size.x) - size.x);
	for (unsigned int y = 0; y < size.y; ++y) {
		unsigned int base = y * (size.x-1 + size.x);
		for (unsigned int x = 0; x + 1 < size.x; ++x) {
			perm[base + x] = 0.5 * (col(y * size.x + x).a + col(y * size.x + x + 1).a);
		}
		base += size.x-1;
		if (y + 1 < size.y) {
			for (unsigned int x = 0; x < size.x; ++x) {
				perm[base + x] = 0.5 * (col(y * size.x + x).a + col((y+1) * size.x + x).a);
			}
		}
	}
}

float w(Vector3f a, Vector3f b) {
	const double Alpha = 1.0;
	const double Epsilon = 1e-5;
	float diff = fabsf(a.x - b.x) + fabsf(a.y - b.y) + fabsf(a.z - b.z);
	return 1.0 / (pow(diff, Alpha) + Epsilon);
}

void Layer::perm_from_color(Vector2ui size) {
	assert(size.x * size.y == data.size());
	perm.resize(size.y * (size.x-1 + size.x) - size.x);
	for (unsigned int y = 0; y < size.y; ++y) {
		unsigned int base = y * (size.x-1 + size.x);
		for (unsigned int x = 0; x + 1 < size.x; ++x) {
			perm[base + x] = w(col(y * size.x + x).xyz, col(y * size.x + x + 1).xyz);
		}
		base += size.x-1;
		if (y + 1 < size.y) {
			for (unsigned int x = 0; x < size.x; ++x) {
				perm[base + x] = w(col(y * size.x + x).xyz, col((y+1) * size.x + x).xyz);
			}
		}
	}
}


//Data that needs to exist as long as the SolveStatus or the solver thread exists:
class SolverData {
public:
	SolverData() : p(NULL), solution_lock(NULL), thread(NULL), suicide(false), did_end(false),  soln_size(make_vector(0U, 0U)), do_soln_coefs(false), soln_serial(0) {
	}
	//nobody writes:
	Problem *p;
	SDL_mutex *solution_lock;
	SDL_Thread *thread;
	//solvestatus writes (unprotected):
	bool suicide;
	//solve thread writes (unprotected):
	bool did_end;
	//solve thread writes (solution_lock):
	vector< uint32_t > soln;
	Vector2ui soln_size;
	bool do_soln_coefs;
	vector< vector< uint32_t > > soln_coefs;
	uint32_t soln_serial;
};

int launch_solve(void *);

class SolveStatusInternal : public SolveStatus {
public:
	SolveStatusInternal(Problem *p, bool save_coefs) {
		data = new SolverData();
		data->p = p;
		data->do_soln_coefs = save_coefs;
		data->suicide = false;
		data->did_end = false;
		data->solution_lock = SDL_CreateMutex();
		data->thread = SDL_CreateThread(launch_solve, data);
	}
	virtual ~SolveStatusInternal() {
		LOG_INFO("Killing off solve thread.");
		data->suicide = true;
		SDL_WaitThread(data->thread, NULL);
		data->thread = NULL;
		SDL_DestroyMutex(data->solution_lock);
		data->solution_lock = NULL;
		delete data->p;
		data->p = NULL;
		delete data;
	}
	virtual bool is_running() {
		return !data->did_end;
	}
	virtual void get_current_solution(vector< uint32_t > &into, Vector2ui &size, vector< vector< uint32_t > > *coefs = NULL) {
		SDL_LockMutex(data->solution_lock);
		into = data->soln;
		size = data->soln_size;
		if (coefs) {
			*coefs = data->soln_coefs;
		}
		SDL_UnlockMutex(data->solution_lock);
	}
	virtual bool get_new_solution(vector< uint32_t > &into, Vector2ui &size, uint32_t &serial, vector< vector< uint32_t > > *coefs) {
		uint32_t new_serial = serial;
		if (serial != data->soln_serial) {
			SDL_LockMutex(data->solution_lock);
			into = data->soln;
			size = data->soln_size;
			if (coefs) {
				*coefs = data->soln_coefs;
			}
			new_serial = data->soln_serial;
			SDL_UnlockMutex(data->solution_lock);
		}
		if (new_serial != serial) {
			serial = new_serial;
			return true;
		} else {
			return false;
		}
	}

	vector< uint32_t > soln;
	Vector2ui soln_size;

	SolverData *data;
};

SolveStatus *start_problem(Problem *p, bool save_coefs) {
	for (unsigned int l = 0; l < p->layers.size(); ++l) {
		if (p->layers[l].perm.size() != p->size.y * (p->size.x-1 + p->size.x) - p->size.x) {
			LOG_INFO("Generating permeability for layer " << l << " from alpha channel.");
			p->layers[l].perm_from_alpha(p->size);
			LOG_INFO(" Have " << p->layers[l].perm.size() << " perm (" << p->size.x << "x" << p->size.y << " image).");
		}
	}
	//Lame way to make sure that there isn't a problem with
	//the static table of factorials in SS_coefs.o when we
	//split off another thread.
	for (unsigned int i = 0; i < 10; ++i) {
		count_stackings(i);
	}
	return new SolveStatusInternal(p, save_coefs);
}


typedef double Real; //need to use double if using pardiso; float is probably better for iterative solve.
//weirdly, double seems faster??


/*
 * Helper wrapper for LAPACK matrix multiply.
 *
 */

void mul(unsigned int m_in, unsigned int k_in, unsigned int n_in, vector< float > const &one, vector< float > const &two, vector< float > &out) {
	assert(one.size() == m_in*k_in);
	assert(two.size() == k_in*n_in);
	out.resize(m_in*n_in);
	{
		//temp <- (two' * one')'
		char transa = 'N';
		char transb = 'N';
		MKL_INT m = n_in;
		MKL_INT n = m_in;
		MKL_INT k = k_in;
		MKL_INT lda = n_in;
		MKL_INT ldb = k_in;
		float alpha = 1.f;
		float beta = 0.f;
		MKL_INT ldc = n_in;
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &two[0], &lda, &one[0], &ldb,
		      &beta, &out[0], &ldc);
	}
}


/*
 * Maps from pixel-wise dofs (taking into account cons) to stackings
 * of layers present at pixel.
 *
 * Note first dof is constant ones.
 */

void dofs_to_stackings(vector< uint32_t > layers, vector< Cons > cons, vector< float > &out) {
	if (cons.empty()) {
		size_t stackings = count_stackings(layers.size());
		out.resize(stackings * stackings, 0.0);
		for (unsigned int i = 0; i < stackings; ++i) {
			out[0 * stackings + i] =-1.0; //first row is 1 -1 .. -1
			out[i * stackings + i] = 1.0; //other rows are 0 .. 0 1 0 .. 0
		}
	} else {
		vector< uint32_t > at_ind;
		for (unsigned int l = 0; l < layers.size(); ++l) {
			while (at_ind.size() <= layers[l]) {
				at_ind.push_back(-1U);
			}
			at_ind[layers[l]] = l;
		}
		
		set< uint32_t > valid_at_orders;
		{
			auto c = cons.begin(); //just take first cons.
			for (unsigned int i = 0; i < c->valid_orders.size(); ++i) {
				vector< uint32_t > order = to_stacking(c->valid_orders[i], c->layers_count);
				vector< uint32_t > at_order;
				for (unsigned int o = 0; o < order.size(); ++o) {
					while (order[o] >= at_ind.size()) {
						at_ind.push_back(-1U);
					}
					if (at_ind[order[o]] != -1U) {
						at_order.push_back(at_ind[order[o]]);
					}
				}
				assert(at_order.size() == layers.size()); //these are all complete orders, right?
				valid_at_orders.insert(to_stacking_index(at_order));
			}
		}

		size_t stackings = count_stackings(layers.size());
		size_t possible = valid_at_orders.size();
		assert(possible > 0 && possible <= stackings);

		size_t cur_dof = 0;
		out.resize(stackings * possible, 0.0);
		for (size_t s = 0; s < stackings; ++s) {
			if (valid_at_orders.count(s)) {
				assert(cur_dof <= possible);
				//this is allowed by the constraint.
				out[s * possible + cur_dof] = 1.0f;
				if (cur_dof == 0) {
					for (unsigned int d = 1; d < possible; ++d) {
						out[s * possible + d] = -1.0f;
					}
				}
				++cur_dof;
			}
		}
		assert(cur_dof == possible);
	}
}

/*
 * Matrix that maps from pixel-wise coefs on stackings of 'layers' to
 * output stackings of 'common_layers'
 */

void stackings_to_stackings(vector< uint32_t > layers, vector< uint32_t > common_layers, vector< float > &out) {
	size_t local_coefs = count_stackings(layers.size());
	size_t common_coefs = count_stackings(common_layers.size());
	out.clear();
	out.resize(local_coefs * common_coefs, 0.0f);

	//Figure out inds in common of all layers (or -1 if do not appear):
	vector< uint32_t > common_ind;
	for (unsigned int i = 0; i < layers.size(); ++i) {
		while (common_ind.size() <= layers[i]) {
			common_ind.push_back(-1U);
		}
	}
	for (unsigned int i = 0; i < common_layers.size(); ++i) {
		assert(common_layers[i] < common_ind.size());
		common_ind[common_layers[i]] = i;
	}

	//For every stacking, add proper coefs:
	for (unsigned int s = 0; s < local_coefs; ++s) {
		Stacking ord = to_stacking(s, layers.size());
		{ //filter out the bits of ord that don't matter:
			auto i = ord.begin();
			while (i != ord.end()) {
				assert(*i < layers.size());
				if (common_ind[layers[*i]] < common_layers.size()) {
					*i = common_ind[layers[*i]];
					++i;
				} else {
					i = ord.erase(i);
				}
			}
		}
		assert(ord.size() == common_layers.size());
		for (unsigned int i = 0; i < ord.size(); ++i) {
			assert(ord[i] < common_layers.size());
		}
		size_t c = to_stacking_index(ord);
		assert(c < common_coefs);
		out[c * local_coefs + s] += 1.0f;
	}
}

/*
 * shortcut for stackings_to_stackings * dofs_to_stackings.
 */

void dofs_to_substackings(vector< uint32_t > layers, vector< Cons > cons, vector< uint32_t > sub_layers, vector< float > &out) {
	vector< float > d_to_s;
	vector< float > s_to_s;
	dofs_to_stackings(layers, cons, d_to_s);
	stackings_to_stackings(layers, sub_layers, s_to_s);
	mul(count_stackings(sub_layers.size()), count_stackings(layers.size()), d_to_s.size() / count_stackings(layers.size()), s_to_s, d_to_s, out);
}


void add_partials(
		vector< uint32_t > at_layers,
		vector< Cons > at_cons,
		vector< uint32_t > other_layers,
		vector< Cons > other_cons,
		vector< uint32_t > common_layers,
		vector< float > common_alphas,
		Real *at_out,
		Real *other_out,
		size_t cols,
		Real *one_out) {
	//Figure out partials of term of objective between at and other.
	assert(at_layers.size() >= common_layers.size());
	assert(other_layers.size() >= common_layers.size());
	assert(!common_layers.empty());

	size_t common_stackings = count_stackings(common_layers.size());

	//figure out what sums of at,other,1 make common stackings:
	vector< float > one_at_to_common, one_other_to_common;
	dofs_to_substackings(at_layers, at_cons, common_layers, one_at_to_common);
	dofs_to_substackings(other_layers, other_cons, common_layers, one_other_to_common);
	assert(one_at_to_common.size() % common_stackings == 0);
	assert(one_other_to_common.size() % common_stackings == 0);
	size_t at_coefs = one_at_to_common.size() / common_stackings - 1;
	size_t other_coefs = one_other_to_common.size() / common_stackings - 1;

	vector< float > one_at_other_to_diffs(common_stackings * (1 + at_coefs + other_coefs));

	//basically it's just at_to + other_to - repeated column of ones:
	for (unsigned int row = 0; row < common_stackings; ++row) {
		size_t row_base = row * (1 + at_coefs + other_coefs);
		one_at_other_to_diffs[row_base + 0] = one_at_to_common[row*(at_coefs+1) + 0] - one_other_to_common[row*(other_coefs+1) + 0];
		for (unsigned int c = 0; c < at_coefs; ++c) {
			one_at_other_to_diffs[row_base+1 + c] = one_at_to_common[row*(at_coefs+1) + 1+c];
		}
		for (unsigned int c = 0; c < other_coefs; ++c) {
			one_at_other_to_diffs[row_base+1+at_coefs + c] =-one_other_to_common[row*(other_coefs+1) + 1+c];
		}
	}
	#ifdef POISSON
	LOG_WARNING("**** USING POISSON SOLVE *****");
	for (unsigned int i = 0; i < common_alphas.size(); ++i) {
		common_alphas[i] = 1.0f;
	}
	#endif //POISSON

	//partials in terms of at,other (also 1, I guess):
	vector< float > partials;
	build_factors_matrix(common_alphas, partials);
	//we want partials of:
	// (weighted_diffs_to_deltas * one_at_other_to_diffs * [1 at common]')^2
	//which would be:
	// one_at_other_to_diffs' * partials * one_at_other_to_diffs
	vector< float > temp(common_stackings * (1+at_coefs+other_coefs));
	{
		//temp <- (one_at_other_to_diffs' * partials')'
		char transa = 'N';
		char transb = 'N';
		MKL_INT m = (1+at_coefs+other_coefs);
		MKL_INT n = common_stackings;
		MKL_INT k = common_stackings;
		MKL_INT lda = (1+at_coefs+other_coefs);
		MKL_INT ldb = common_stackings;
		float alpha = 1.f;
		float beta = 0.f;
		MKL_INT ldc = (1+at_coefs+other_coefs);
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &one_at_other_to_diffs[0], &lda, &partials[0], &ldb,
		      &beta, &temp[0], &ldc);

	}
	vector< float > full_partials((1+at_coefs+other_coefs) * (1+at_coefs+other_coefs));
	{
		//full_partials <- (temp' * one_at_other_to_diffs)'
		char transa = 'N';
		char transb = 'T';
		MKL_INT m = (1+at_coefs+other_coefs);
		MKL_INT n = (1+at_coefs+other_coefs);
		MKL_INT k = common_stackings;
		MKL_INT lda = (1+at_coefs+other_coefs);
		MKL_INT ldb = (1+at_coefs+other_coefs);
		float alpha = 1.f;
		float beta = 0.f;
		MKL_INT ldc = (1+at_coefs+other_coefs);
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &temp[0], &lda, &one_at_other_to_diffs[0], &ldb,
		      &beta, &full_partials[0], &ldc);
	}

	//Transfer the bit of full_partials we care about into output arrays:
	for (size_t row = 0; row < at_coefs; ++row) {
		size_t row_base = (row+1)*(1+at_coefs+other_coefs);
		one_out[row] += full_partials[row_base + 0];
		for (size_t c = 0; c < at_coefs; ++c) {
			at_out[row*cols + c] += full_partials[row_base + 1+c];
		}
		for (size_t c = 0; c < other_coefs; ++c) {
			other_out[row*cols + c] += full_partials[row_base + 1+at_coefs+c];
		}
	}
}

void test_add_partials() {
	//-------------------------
	//Dump some examples of partials matrix...
	{
		float test_alphas[] = {
			1.0, 1.0, -1.0,
			1.0, 0.5, -1.0,
			0.5, 0.5, -1.0,
			1.0, 1.0, 1.0, -1.0,
			1.0, 1.0, 0.1, -1.0,
			1.0, 1.0, 1.0, 1.0, -1.0,
			-1.0
		};
		for (unsigned int ta = 0; test_alphas[ta] != -1.0; ) {
			vector< float > alphas;
			while (test_alphas[ta] != -1.0) {
				alphas.push_back(test_alphas[ta]);
				++ta;
			}
			++ta;
			LOG_INFO("Alphas:");
			row_dump(&alphas[0], 1, alphas.size());
			vector< float > partials;
			build_factors_matrix(alphas, partials);
			assert(partials.size() == count_stackings(alphas.size()) * count_stackings(alphas.size()));
			LOG_INFO("give partials:");
			row_dump(&partials[0], count_stackings(alphas.size()), count_stackings(alphas.size()));
		}
	}
	{//add_partials bit...
	uint32_t test_at_layers[] = {
		0, 1, -1U,
		0, 1, -1U,
		0, 1, 2, -1U,
		0, 1, 2, 3, -1U,
		-1U,
	};
	uint32_t test_other_layers[] = {
		0, 1, -1U,
		0, 1, -1U,
		0, 1, 2, -1U,
		0, 1, 2, 3, -1U,
		-1U,
	};
	float test_alphas[] = {
		1.0, 1.0, -1.0,
		0.2, 1.0, -1.0,
		1.0, 1.0, 1.0, -1.0,
		1.0, 1.0, 1.0, 1.0, -1.0,
		-1.0,
	};

	size_t at_i = 0;
	size_t other_i = 0;
	size_t alphas_i = 0;
	while (1) {
		if (test_at_layers[at_i] == -1U) {
			assert(test_other_layers[other_i] == -1U);
			assert(test_alphas[alphas_i] == -1.0);
			break;
		}
		vector< uint32_t > at_layers;
		vector< uint32_t > other_layers;
		vector< float > alphas;
		while (test_at_layers[at_i] != -1U) {
			at_layers.push_back(test_at_layers[at_i]);
			++at_i;
		}
		++at_i;
		while (test_other_layers[other_i] != -1U) {
			other_layers.push_back(test_other_layers[other_i]);
			++other_i;
		}
		++other_i;
		while (test_alphas[alphas_i] != -1.0) {
			alphas.push_back(test_alphas[alphas_i]);
			++alphas_i;
		}
		++alphas_i;
		vector< uint32_t > common_layers;
		vector< float > common_alphas;
		{ //determine common layers and their alphas:
			vector< bool > at_has;
			for (unsigned int i = 0; i < at_layers.size(); ++i) {
				while (at_layers[i] >= at_has.size()) {
					at_has.push_back(false);
				}
				at_has[at_layers[i]] = true;
			}
			for (unsigned int i = 0; i < other_layers.size(); ++i) {
				if (other_layers[i] < at_has.size() && at_has[other_layers[i]]) {
					assert(other_layers[i] < alphas.size());
					common_layers.push_back(other_layers[i]);
					common_alphas.push_back(alphas[other_layers[i]]);
				}
			}
		}

		LOG_INFO("With at_layers:");
		row_dump(&at_layers[0], 1, at_layers.size());
		LOG_INFO("  other_layers:");
		row_dump(&other_layers[0], 1, other_layers.size());
		LOG_INFO("  common_layers:");
		row_dump(&common_layers[0], 1, common_layers.size());
		LOG_INFO("  common_alphas:");
		row_dump(&common_alphas[0], 1, common_alphas.size());

		size_t at_coefs = count_stackings(at_layers.size())-1;
		size_t cols = at_coefs+count_stackings(other_layers.size())-1;
		vector< Real > at_other(at_coefs * cols, 0.0);
		vector< Real > ones(at_coefs, 0.0);
		add_partials(at_layers, vector< Cons >(), other_layers, vector< Cons >(), common_layers, common_alphas,
			&(at_other[0]), &(at_other[at_coefs]), cols,
			&(ones[0]));
		LOG_INFO(" have  at | other:");
		row_dump(&at_other[0], at_coefs, cols);
		LOG_INFO("  ones:");
		row_dump(&ones[0], at_coefs, 1);
		float max_mag = 0.0;
		for (unsigned int r = 0; r < at_coefs; ++r) {
			float sum = ones[r];
			for (unsigned int c = 0; c < cols; ++c) {
				sum += at_other[r * cols + c];
			}
			sum = fabs(sum);
			if (sum > max_mag) max_mag = sum;
		}
		LOG_INFO(" Max row sum is: " << max_mag);
	}
	} //end add_partials bit of test.
	LOG_INFO(" ----------- end of tests --------------");
}

void profile_solution(
		vector< size_t > const &starts,
		vector< int > const &row_inds,
		vector< int > const &inds,
		vector< Real > const &coefs,
		vector< Real > const &rhs,
		vector< Real > const &soln) {

	//check how good a solution this is:
	Real max_diff = Real(0.0);
	for (unsigned int row = 0; row + 1 < row_inds.size(); ++row) {
		Real sum = Real(0.0);
		assert((unsigned)row_inds[row] - 1 <= inds.size());
		assert((unsigned)row_inds[row+1] - 1 <= inds.size());
		for (unsigned int ind = row_inds[row]-1; ind < (unsigned)row_inds[row+1]-1; ++ind) {
			unsigned int i = inds[ind]-1;
			assert(i < soln.size());
			sum += coefs[ind] * soln[i];
		}
		assert(row < rhs.size());
		Real diff = fabs(rhs[row] - sum);
		if (diff > max_diff) {
			max_diff = diff;
		}
	}
	LOG_INFO("Solution is accurate to within " << max_diff << ".");

	//check the sum-invariance that we so desperately desire:
	Real min_val = 1.0;
	Real max_val = 0.0;
	unsigned int good_vals = 0;
	unsigned int bad_vals = 0;
	Real min_sum = 1.0;
	Real max_sum = 0.0;
	unsigned int good_sums = 0;
	unsigned int bad_sums = 0;
	for (unsigned int i = 0; i + 1 < starts.size(); ++i) {
		Real sum = 0.0;
		for (unsigned int si = starts[i]; si < starts[i+1]; ++si) {
			if (soln[si] < min_val) {
				min_val = soln[si];
			}
			if (soln[si] > max_val) {
				max_val = soln[si];
			}
			if (soln[si] >= 0.0 && soln[si] <= 1.0) {
				good_vals += 1;
			} else {
				bad_vals += 1;
			}
			sum += soln[si];
		}
		if (sum > max_sum) {
			max_sum = sum;
		}
		if (sum < min_sum) {
			min_sum = sum;
		}
		if (starts[i] != starts[i+1]) {
			if (sum >= 0 && sum <= 1) {
				good_sums += 1;
			} else {
				bad_sums += 1;
			}
		}
	}

	LOG_INFO(" Min sum: " << min_sum << ", max_sum: " << max_sum << " (ideally this would be 0,1).");
	LOG_INFO("  ... but had " << good_sums << " non-trivial good sums versus " << bad_sums << " bad ones.");

	LOG_INFO(" Min val: " << min_val << ", max_val: " << max_val << " (ideally this would be 0,1).");
	LOG_INFO("  ... but had " << good_vals << " non-trivial good vals versus " << bad_vals << " bad ones.");
}

void write_solution(SolverData *sd, Problem &prob, std::string filename, vector< size_t > const &starts, vector< Real > const &soln) {
	std::cout << "Rendering '" << filename << "'..."; std::cout.flush();
	assert(starts.size() == prob.size.x * prob.size.y + 1);
	vector< uint32_t > out_img(prob.size.x * prob.size.y);
	vector< vector< uint32_t > > soln_coefs_storage;
	vector< vector< uint32_t > > *soln_coefs = NULL;
	if (sd->do_soln_coefs) {
		soln_coefs = &(soln_coefs_storage);
		soln_coefs_storage.resize(count_stackings(prob.layers.size()));
	}
	if (soln_coefs) {
		for (vector< vector< uint32_t > >::iterator sc = soln_coefs->begin(); sc != soln_coefs->end(); ++sc) {
			sc->resize(prob.size.x * prob.size.y, 0xff000000U);
		}
	}
	for (size_t p = 0; p < prob.size.x * prob.size.y; ++p) {
		Vector4f sum = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
		vector< uint32_t > inds = prob.layers_at(p);
		vector< Vector4f > cols;
		for (unsigned int l = 0; l < inds.size(); ++l) {
			Vector4f col;
			uint32_t px = prob.layers[inds[l]].data[p];
			for (unsigned int i = 0; i < 4; ++i) {
				col[i] = ((px >> (8 * i)) & 0xff) / 255.0f;
			}
			cols.push_back(col);
		}
		if (!cols.empty()) {
			vector< float > coefs(count_stackings(cols.size()), 0.0f);
			{ //multiply dofs out to get coefs:
				size_t p_dofs = starts[p+1] - starts[p];
				vector< float > p_to_stackings;
				dofs_to_stackings(inds, prob.constraints_at(p), p_to_stackings);
				assert(p_to_stackings.size() == (p_dofs+1) * count_stackings(cols.size()));
				for (unsigned int s = 0; s < coefs.size(); ++s) {
					coefs[s] += p_to_stackings[s*(p_dofs+1) + 0];
					for (unsigned int p_dof = 0; p_dof < p_dofs; ++p_dof) {
						coefs[s] += p_to_stackings[s*(p_dofs+1) + 1 + p_dof] * soln[starts[p] + p_dof];
					}
				}
			}
			vector< uint32_t > not_inds;
			if (soln_coefs) {
				vector< bool > has(prob.layers.size(), false);
				for (vector< uint32_t >::iterator i = inds.begin(); i != inds.end(); ++i) {
					assert(*i < has.size());
					assert(has[*i] == false);
					has[*i] = true;
				}
				for (uint32_t i = 0; i < has.size(); ++i) {
					if (!has[i]) {
						not_inds.push_back(i);
					}
				}
			}
			for (unsigned int st = 0; st < coefs.size(); ++st) {
				if (coefs[st] == 0.0) continue;
				Stacking order = to_stacking(st, cols.size());
				Vector4f comp = make_vector(0.0f, 0.0f, 0.0f, 0.0f);
				for (unsigned int l = 0; l < order.size(); ++l) {
					Vector4f col = cols[order[l]];
					comp.xyz = (col.xyz * col.w + comp.xyz * (1 - col.w) * comp.w) / (col.w + (1 - col.w) * comp.w);
					comp.w = (1 - comp.w) * col.w + comp.w;
				}

				sum += coefs[st] * comp;

				if (soln_coefs) {
					Stacking full_order = not_inds;
					for (unsigned int l = 0; l < order.size(); ++l) {
						assert(order[l] < inds.size());
						full_order.push_back(inds[order[l]]);
					}
					assert(full_order.size() == prob.layers.size());
					unsigned int idx = to_stacking_index(full_order);
					assert(idx < soln_coefs->size());
					int val = coefs[st] * 256;
					if (val > 255) val = 255;
					if (val < 0) val = 0;
					(*soln_coefs)[idx][p] = 0xff000000 | (val << 16) | (val << 8) | val;
				}
			}
		}
		uint32_t px = 0;
		for (unsigned int i = 0; i < 4; ++i) {
			int32_t v = sum[i] * 256;
			if (v > 255) v = 255;
			if (v < 0) v = 0;
			px = px | (v << (8 * i));
		}
		out_img[p] = px;
	}
	std::cout << " (not actually saving) "; std::cout.flush();
	//save_png(filename, prob.size.x, prob.size.y, &out_img[0]);
	SDL_LockMutex(sd->solution_lock);
	sd->soln_serial += 1;
	sd->soln = out_img;
	sd->soln_size = prob.size;
	sd->soln_coefs = soln_coefs_storage;
	SDL_UnlockMutex(sd->solution_lock);
	std::cout << "done." << std::endl;
}

void make_starts(Problem &prob, vector< size_t > &starts) {
	//figure out how many variables at each pixel & allocate storage for system:
	starts.resize(prob.size.x*prob.size.y + 1);
	{
		size_t cur_start = 0;
		for (unsigned int p = 0; p < prob.size.x * prob.size.y; ++p) {
			starts[p] = cur_start;
			vector< uint32_t > l = prob.layers_at(p);
			if (l.size() <= 1) continue;
			vector< Cons > c = prob.constraints_at(p);
			if (c.empty()) {
				//one dof per stacking, and they sum to one.
				cur_start += count_stackings(l.size()) - 1;
			} else {
				vector< float > d_to_s;
				dofs_to_stackings(l, c, d_to_s);
				assert(d_to_s.size() % count_stackings(l.size()) == 0);
				assert(d_to_s.size() / count_stackings(l.size()) >= 1);
				cur_start += d_to_s.size() / count_stackings(l.size()) - 1;
			}
		}
		starts.back() = cur_start;
	}
	LOG_INFO("Have " << starts.back() << " variables.");
}

void run_solve(SolverData *sd, Problem &prob, vector< size_t > const &starts, vector< Real > &soln, unsigned int max_iter /*= -1U*/, Real tolerance /*= 0.00001 / 256.0*/, Timer &timer) {

	timer.time_point("build", prob.size);

	assert(starts.size() == prob.size.x * prob.size.y + 1);
	assert(soln.size() == starts.back());

	const unsigned int Ofs = 5;
	Vector2ui ofs[Ofs] = {
		{{{ 0,-1U}}},
		{{{-1U, 0}}},
		{{{ 0, 0}}},
		{{{ 1, 0}}},
		{{{ 0, 1}}},
	};

	vector< Real > coefs; //value for nonzeros in each row -- 1-based.
	vector< int > inds; //col. number of nonzeros in each row -- 1-based.
	vector< int > row_inds; //start of each row -- 1-based, again.
	vector< Real > rhs; //right-hand-side for system
	//figure out how much storage based on common layers among neighbors:
	{
		size_t storage = 0;
		Vector2ui p;
		for (p.y = 0; p.y < prob.size.y; ++p.y) {
			for (p.x = 0; p.x < prob.size.x; ++p.x) {
				size_t cols = 0;
				for (unsigned int i = 0; i < Ofs; ++i) {
					Vector2ui np = p + ofs[i];
					if (np.x >= prob.size.x) continue;
					if (np.y >= prob.size.y) continue;
					size_t ni = np.x + np.y * prob.size.x;
					cols += starts[ni+1] - starts[ni];
				}
				size_t pi = p.x + p.y * prob.size.x;
				storage += cols * (starts[pi+1] - starts[pi]);
			}
		}
		LOG_INFO("Will require " << storage << " floats to store matrix.");
		coefs.reserve(storage);
		inds.reserve(storage);
		row_inds.reserve(starts.back()+1);
		rhs.reserve(starts.back());
	}
	//build actual matrix:
	{
		std::cout << "Building matrix..."; std::cout.flush();
		Vector2ui p;
		for (p.y = 0; p.y < prob.size.y; ++p.y) {
			for (p.x = 0; p.x < prob.size.x; ++p.x) {
				{
					//make some dots to indicate we're actually doing something
					size_t dot = (p.y * prob.size.x + p.x) * 80 / (prob.size.x * prob.size.y);
					size_t next_dot = (p.y * prob.size.x + p.x + 1) * 80 / (prob.size.x * prob.size.y);
					if (dot != next_dot) {
						std::cout << ".";
						std::cout.flush();
					}


				}
				//figure out how many columns of storage this pixel needs for its neighbors; also record where each column starts:
				size_t cols = 0;
				size_t ofs_col[Ofs] = { -1U, -1U, -1U, -1U, -1U };
				for (unsigned int i = 0; i < Ofs; ++i) {
					Vector2ui np = p + ofs[i];
					if (np.x >= prob.size.x) continue;
					if (np.y >= prob.size.y) continue;
					size_t ni = np.x + np.y * prob.size.x;
					ofs_col[i] = cols;
					cols += starts[ni+1] - starts[ni];
				}

				//Now build a local matrix with the proper number of columns in which to store our partials:
				size_t pi = p.x + p.y * prob.size.x;
				vector< Real > local_mat(cols * (starts[pi+1] - starts[pi]), 0.0f);
				vector< Real > local_ones(starts[pi+1] - starts[pi], 0.0f);
				if (local_mat.empty()) continue;

				//Determine what layers are here, build p_has to make finding
				//common layers easier.
				vector< uint32_t > p_layers = prob.layers_at(p);
				vector< Cons > p_cons = prob.constraints_at(p);
				//vector< float > p_alphas = prob.alphas_at(p);
				vector< bool > p_has(prob.layers.size(), false);
				for (unsigned int l = 0; l < p_layers.size(); ++l) {
					assert(p_layers[l] < p_has.size());
					p_has[p_layers[l]] = true;
				}
				{ //do partials
					for (unsigned int i = 0; i < Ofs; ++i) {
						Vector2ui np = p + ofs[i];
						if (np == p) continue;
						if (ofs_col[i] > cols) continue;
						//For every neighbor, find common layers...
						vector< uint32_t > other_layers = prob.layers_at(np);
						vector< Cons > other_cons = prob.constraints_at(np);
						//vector< float > other_alphas = prob.alphas_at(np);
						vector< uint32_t > common_layers;
						for (auto o = other_layers.begin(); o != other_layers.end(); ++o) {
							assert(*o < p_has.size());
							if (p_has[*o]) {
								common_layers.push_back(*o);
							}
						}
						if (common_layers.size() < 2) continue; //I guess this part was zero after all!
						vector< float > all_alphas = prob.alphas_between(p, np);
						vector< float > common_alphas(common_layers.size());
						for (unsigned int c = 0; c < common_layers.size(); ++c) {
							common_alphas[c] = all_alphas[common_layers[c]];
							//common_alphas[c] = 0.5 * (p_alphas[common_layers[c]] + other_alphas[common_layers[c]]);
						}
						//Once we know common layers and their alphas we can build
						//the proper tidbit of the objective function:
						//DEBUG: no partials.
						#ifdef NO_PARTIALS
						{
							static bool warned = false;
							if (!warned) {
								warned = true;
								LOG_WARNING("***BEWARE***");
								LOG_WARNING("***DEBUG**** not actually adding in any partials for continuity.");
								LOG_WARNING("***DANGER***");
							}
						}
						#else
						add_partials(p_layers, p_cons, other_layers, other_cons, common_layers, common_alphas, &(local_mat[ofs_col[2]]), &(local_mat[ofs_col[i]]), cols, &(local_ones[0]));
						#endif
					}
				} //do partials
				//Local mat, ones -> global matrix, rhs
				for (size_t local_row = 0; local_row < (starts[pi+1] - starts[pi]); ++local_row) {
					rhs.push_back(-local_ones[local_row]);
					row_inds.push_back(inds.size()+1);
					for (unsigned int n = 0; n < Ofs; ++n) {
						if (ofs_col[n] == -1U) continue;

						Vector2ui np = p + ofs[n];
						assert(np.x < prob.size.x);
						assert(np.y < prob.size.y);
						size_t ni = np.x + np.y * prob.size.x;


						for (size_t local_col = ofs_col[n]; local_col < ofs_col[n] + starts[ni+1] - starts[ni]; ++local_col) {
							assert(local_col <= cols);
							float coef = local_mat[local_row * cols + local_col];
							if (coef == 0.0) continue;
							size_t full_col = starts[ni] + (local_col - ofs_col[n]);
							inds.push_back(full_col + 1);
							coefs.push_back(coef);
						}
					} //for neighbor
				} //for local_row
			} //for x
			if (sd->suicide) return;
		} //for y
		std::cout << " done." << std::endl;
		row_inds.push_back(inds.size()+1);
		assert(rhs.size() == starts.back());
		assert(row_inds.size() == rhs.size() + 1);
		assert(inds.size() == coefs.size());
		LOG_INFO("System actually has " << inds.size() << " non-zeros.");
	} //"build matrix" scope

	timer.time_point("iter", prob.size);
	//#define DIRECT_SOLVE
	#ifdef DIRECT_SOLVE
	std::cout << "Solving... "; std::cout.flush();
	//Call MKL:
	{
		_MKL_DSS_HANDLE_t handle;
		int OPT = (MKL_DSS_MSG_LVL_INFO + MKL_DSS_TERM_LVL_ERROR + MKL_DSS_OOC_VARIABLE);
		int ret = dss_create(handle, OPT);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on create: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
		int OPT2 = MKL_DSS_NON_SYMMETRIC;
		int nRows = rhs.size();
		int nCols = rhs.size();
		int nNonZeros = inds.size();
		ret = dss_define_structure(handle, OPT2, &(row_inds[0]), nRows, nCols, &(inds[0]), nNonZeros);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on define: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
		int OPT3 = MKL_DSS_AUTO_ORDER;
		ret = dss_reorder(handle, OPT3, NULL);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on reorder: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
		int OPT4 = MKL_DSS_INDEFINITE;
		ret = dss_factor_real(handle, OPT4, &coefs[0]);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on factor: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
		//solve:
		int nRhs = 1;
		ret = dss_solve_real(handle, OPT, &rhs[0], nRhs, &soln[0]);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on solve: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
		//clean up:
		ret = dss_delete(handle, OPT);
		if (ret != MKL_DSS_SUCCESS) {
			LOG_ERROR("Error on delete: " << ret);
			assert(ret == MKL_DSS_SUCCESS);
		}
	} //end of "call MKL" part.
	std::cout << "  Done." << std::endl;

	profile_solution(starts, row_inds, inds, coefs, rhs, soln);
	write_solution("out.png", starts, soln);
	#endif

	#define ITERATIVE_SOLVE
	#ifdef ITERATIVE_SOLVE
	std::cout << "Iterative solve..." << std::endl;
	{
		Real max_delta = tolerance + 1.0;
		unsigned int iters = 0;
		while (!(max_delta < tolerance) && iters < max_iter) {
			max_delta = Real(0.0);
			unsigned int start_start = (iters & 0x1)?(starts.size()-2):0;
			int start_step = (iters & 0x1)?-1:1;
			assert(!starts.empty()); //otherwise starts.size()-1 is a mistake.
			for (unsigned int start = start_start; start < starts.size()-1; start += start_step) {
				unsigned int ri_max = starts[start+1]-starts[start];
				unsigned int ri_start = (iters & 0x1)?(ri_max-1):0;
				int ri_step = (iters & 0x1)?-1:1;
				Real start_sum = Real(0.0);
				for (unsigned int ri = ri_start; ri < ri_max; ri += ri_step) {
					unsigned int row = starts[start] + ri;
					assert((unsigned)row_inds[row] - 1 <= inds.size());
					assert((unsigned)row_inds[row+1] - 1 <= inds.size());
					Real sum = -rhs[row];
					Real inv = Real(0.0);
					for (unsigned int ind = row_inds[row]-1; ind < (unsigned)row_inds[row+1]-1; ++ind) {
						unsigned int i = inds[ind]-1;
						assert(i < soln.size());
						if (i == row) {
							inv =Real(-1.0) / coefs[ind];
						} else {
							sum += coefs[ind] * soln[i];
						}
					}
					//SOR:
					Real next = soln[row] + Real(1.5) * (inv * sum - soln[row]);
					if (next > Real(1.0)) next = Real(1.0);
					if (next < Real(0.0)) next = Real(0.0);
					start_sum += next;
					Real delta = fabs(soln[row] - next);
					if (delta > max_delta) max_delta = delta;
					soln[row] = next;
				}
				if (start_sum > Real(1.0)) {
					Real inv = Real(1.0) / start_sum;
					for (unsigned int ri = ri_start; ri < ri_max; ri += ri_step) {
						soln[starts[start] + ri] *= inv;
					}
				}
			}
			++iters;
			LOG_INFO(iters << " - max_delta: " << max_delta);
			if (iters % 100 == 0) {
				//profile_solution(starts, row_inds, inds, coefs, rhs, soln);
				write_solution(sd, prob, "out-iter.png", starts, soln);
			}
			if (sd->suicide) return;
		}
	}
	std::cout << "  Done." << std::endl;

	//profile_solution(starts, row_inds, inds, coefs, rhs, soln);
	write_solution(sd, prob, "out-iter.png", starts, soln);
	#endif

}

void downsample(Vector2ui size, vector< uint32_t > const &data, Vector2ui small_size, vector< uint32_t > &small) {
	assert((small_size.x - 1) * 2 < size.x);
	assert((small_size.y - 1) * 2 < size.y);
	small.resize(small_size.x * small_size.y);
	Vector2ui p;
	for (p.y = 0; p.y < small_size.y; ++p.y) {
		for (p.x = 0; p.x < small_size.x; ++p.x) {
			size_t li = p.y*2*size.x + p.x*2;
			assert(li < data.size());
			#define COL(I) make_vector< float >( data[(I)] >> 24, (data[(I)] >> 16) & 0xff, (data[(I)] >> 8) & 0xff, data[(I)] & 0xff )
			Vector4f val = COL(li);
			float div = 1.0;
			if (p.x*2-1 < size.x) {
				val += 0.5f * COL(li-1);
				div += 0.5f;
				if (p.y*2-1 < size.y) {
					val += 0.25f * COL(li-1-size.x);
					div += 0.25f;
				}
				if (p.y*2+1 < size.y) {
					val += 0.25f * COL(li-1+size.x);
					div += 0.25f;
				}
			}
			if (p.x*2+1 < size.x) {
				val += 0.5f * COL(li+1);
				div += 0.5f;
				if (p.y*2-1 < size.y) {
					val += 0.25f * COL(li+1-size.x);
					div += 0.25f;
				}
				if (p.y*2+1 < size.y) {
					val += 0.25f * COL(li+1+size.x);
					div += 0.25f;
				}
			}
			if (p.y*2-1 < size.y) {
				val += 0.5f * COL(li-size.x);
				div += 0.5f;
			}
			if (p.y*2+1 < size.y) {
				val += 0.5f * COL(li+size.x);
				div += 0.5f;
			}
			#undef COL

			val /= div;
			uint32_t res = 0;
			for (unsigned int i = 0; i < 4; ++i) {
				int temp = (int)val[i];
				if (temp < 0) temp = 0;
				if (temp > 255) temp = 255;
				res |= (temp << (8 * (3-i)));
			}
			small[p.y * small_size.x + p.x] = res;
		}
	}
}

void downsample_perm(Vector2ui size, vector< float > const &perm, Vector2ui small_size, vector< float > &small_perm) {
	assert((small_size.x - 1) * 2 < size.x);
	assert((small_size.y - 1) * 2 < size.y);
	assert(perm.size() == (size.x-1 + size.x) * size.y - size.x);
	small_perm.resize((small_size.x-1 + small_size.x) * small_size.y - small_size.x);
	for (unsigned int y = 0; y < small_size.y; ++y) {
		unsigned int small_base = y * (small_size.x-1 + small_size.x);
		for (unsigned int x = 0; x + 1 < small_size.x; ++x) {
			float amt = 0.0;
			float div = 0.0;
			if (y > 0) {
				amt += 0.25 * perm[(y * 2-1) * (size.x-1 + size.x) + (x * 2)];
				div += 0.25;
				amt += 0.25 * perm[(y * 2-1) * (size.x-1 + size.x) + (x * 2 + 1)];
				div += 0.25;
			}
			amt += 0.5 * perm[(y * 2) * (size.x-1 + size.x) + (x * 2)];
			div += 0.5;
			amt += 0.5 * perm[(y * 2) * (size.x-1 + size.x) + (x * 2 + 1)];
			div += 0.5;
			if (2*y+1 < size.y) {
				amt += 0.25 * perm[(y * 2+1) * (size.x-1 + size.x) + (x * 2)];
				div += 0.25;
				amt += 0.25 * perm[(y * 2+1) * (size.x-1 + size.x) + (x * 2 + 1)];
				div += 0.25;
			}
			small_perm[small_base + x] = amt / div;
		}
		small_base += small_size.x-1;
		if (y + 1 < small_size.y) {
			assert(y * 2 + 1 < size.y);
			for (unsigned int x = 0; x < small_size.x; ++x) {
				float amt = 0.0;
				float div = 0.0;

				if (x > 0) {
					amt += 0.25 * perm[(y * 2) * (size.x-1 + size.x) + size.x-1 + (x * 2 - 1)];
					div += 0.25;
					amt += 0.25 * perm[(y * 2 + 1) * (size.x-1 + size.x) + size.x-1 +  (x * 2 - 1)];
					div += 0.25;
				}
				amt += 0.5 * perm[(y * 2) * (size.x-1 + size.x) + size.x-1 + (x * 2)];
				div += 0.5;
				amt += 0.5 * perm[(y * 2 + 1) * (size.x-1 + size.x) + size.x-1 +  (x * 2)];
				div += 0.5;
				if (2*x+1 < size.x) {
					amt += 0.25 * perm[(y * 2) * (size.x-1 + size.x) + size.x-1 + (x * 2 + 1)];
					div += 0.25;
					amt += 0.25 * perm[(y * 2 + 1) * (size.x-1 + size.x) + size.x-1 +  (x * 2 + 1)];
					div += 0.25;
				}

				small_perm[small_base + x] = amt / div;
			}
		}
	}
}


void Problem::downsample_from(Problem &prob, SolverData *sd) {
	layers.clear();
	constraints.clear();
	size.x = ((int)prob.size.x - 1) / 2 + 1;
	size.y = ((int)prob.size.y - 1) / 2 + 1;
	layers.resize(prob.layers.size());
	for (unsigned int l = 0; l < prob.layers.size(); ++l) {
		downsample(prob.size, prob.layers[l].data, size, layers[l].data);

		downsample_perm(prob.size, prob.layers[l].perm, size, layers[l].perm);
		
		if (sd->suicide) return;
	}
	constraints.resize(prob.constraints.size());
	for (unsigned int c = 0; c < prob.constraints.size(); ++c) {
		constraints[c].valid_orders = prob.constraints[c].valid_orders;
		downsample(prob.size, prob.constraints[c].data, size, constraints[c].data);
		if (sd->suicide) return;
	}
}


void multiscale_solve(SolverData *sd, Problem &prob, vector< size_t > &starts, vector< Real > &soln, Timer &timer) {
	timer.time_point("enter", prob.size);
	make_starts(prob, starts);
	soln.resize(starts.back(), Real(0.0));
	if (prob.size.x > 10 || prob.size.y > 10) {
		Problem sprob;
		//(a) downsample
		timer.time_point("downsample", prob.size);
		sprob.downsample_from(prob, sd);
		if (sd->suicide) return;
		
		//(b) solve
		vector< size_t > small_starts;
		vector< Real > small_soln;
		multiscale_solve(sd, sprob, small_starts, small_soln, timer);

		if (sd->suicide) return;

		//(c) upsample
		timer.time_point("upsample", prob.size);
		{
			/*
			{ //DEBUG
				std::ostringstream fn;
				fn << "m-" << size.x << "x" << size.y << "-after.png";
				write_solution(fn.str(), small_starts, small_soln);
			}
			*/

			std::cout << "Upsampling..."; std::cout.flush();

			//pull out layers_at and constraints_at each small-image part:
			vector< vector< uint32_t > > small_layers(small_starts.size()-1);
			vector< vector< Cons > > small_cons(small_starts.size()-1);
			for (unsigned int s = 0; s + 1 < small_starts.size(); ++s) {
				small_layers[s] = sprob.layers_at(s);
				small_cons[s] = sprob.constraints_at(s);
			}
			//and pull from small-size:
			for (unsigned int s = 0; s + 1 < starts.size(); ++s) {
				vector< uint32_t > large_layers = prob.layers_at(s);

				if (large_layers.empty()) continue;

				unsigned int small_s = (s / prob.size.x) / 2 * sprob.size.x + (s % prob.size.x) / 2;
				assert(small_s < small_starts.size() && small_s + 1 < small_starts.size());

				vector< uint32_t > all_layers;
				{
					uint32_t has = 0;
					for (unsigned int l = 0; l < small_layers[small_s].size(); ++l) {
						has |= (1 << small_layers[small_s][l]);
					}
					for (unsigned int l = 0; l < large_layers.size(); ++l) {
						has |= (1 << large_layers[l]);
					}
					for (unsigned int l = 0; l < prob.layers.size(); ++l) {
						if (has & (1 << l)) {
							all_layers.push_back(l);
						}
					}
				}

				//okay, here's the tricky part:
				//We've got some small coefs that map via small_to_stackings to a subset of stackings of all_layers
				//We want to map these guys over to a different subset of all_layers and then to some different dofs.


				vector< float > dofs_to_small;
				vector< float > dofs_to_large;
				dofs_to_stackings(small_layers[small_s], small_cons[small_s], dofs_to_small);
				dofs_to_stackings(large_layers, prob.constraints_at(s), dofs_to_large);
				assert(dofs_to_small.size() == count_stackings(small_layers[small_s].size()) * (small_starts[small_s+1] - small_starts[small_s] + 1));
				assert(dofs_to_large.size() == count_stackings(large_layers.size()) * (starts[s+1] - starts[s] + 1));
				vector< float > dofs_to_dofs;
				{ //lazy general solution probably not needed 99% of the time:
					//So, one approach would be:
					//pinv(dofs_to_large) * all_to_large * pinv(all_to_small) * dofs_to_small
					vector< float > all_to_large;
					vector< float > all_to_small;
					stackings_to_stackings(all_layers, large_layers, all_to_large);
					stackings_to_stackings(all_layers, small_layers[small_s], all_to_small);
					//easy case; we'll be lazy anyway.
					vector< float > large_to_dofs;
					vector< float > small_to_all;
					{
						vector< float > null_trash;
						pinv_and_null(&dofs_to_large[0], count_stackings(large_layers.size()), starts[s+1] - starts[s] + 1, large_to_dofs, null_trash);
						assert(all_to_small.size() == count_stackings(small_layers[small_s].size()) * count_stackings(all_layers.size()));
						pinv_and_null(&all_to_small[0], count_stackings(small_layers[small_s].size()), count_stackings(all_layers.size()), small_to_all, null_trash);
					}
					vector< float > small_to_large;
					mul(count_stackings(large_layers.size()), count_stackings(all_layers.size()), count_stackings(small_layers[small_s].size()), all_to_large, small_to_all, small_to_large);
					vector< float > small_dofs_to_large;
					mul(count_stackings(large_layers.size()), count_stackings(small_layers[small_s].size()), small_starts[small_s+1] - small_starts[small_s]+1, small_to_large, dofs_to_small, small_dofs_to_large);
					mul(starts[s+1]-starts[s]+1, count_stackings(large_layers.size()), small_starts[small_s+1]-small_starts[small_s]+1, large_to_dofs, small_dofs_to_large, dofs_to_dofs);
				}

				unsigned int small_dofs = small_starts[small_s+1]-small_starts[small_s];
				for (unsigned int dof = 0; dof < starts[s+1] - starts[s]; ++dof) {
					Real sum = Real(0.0);
					sum += dofs_to_dofs[(dof+1)*(small_dofs+1) + 0];
					for (unsigned int s_dof = 0; s_dof < small_dofs; ++s_dof) {
						sum += dofs_to_dofs[(dof+1)*(small_dofs+1) + 1 + s_dof] * small_soln[small_starts[small_s]+s_dof];
					}
					soln[starts[s] + dof] = sum;
				}
				if (sd->suicide) return;
			} 
			std::cout << " done." << std::endl;

			/*
			{ //DEBUG
				std::ostringstream fn;
				fn << "proj-lg-" << size.x << "x" << size.y << ".png";
				write_solution(fn.str(), starts, soln);
			}*/

		}
	}
	/*
	{ //DEBUG
		std::ostringstream fn;
		fn << "m-" << size.x << "x" << size.y << "-before.png";
		write_solution(fn.str(), starts, soln);
	}
	*/
	run_solve(sd, prob, starts, soln, 500, 0.5 / 256.0f, timer); //run 500 iterations or hit threshold
	/*
	{ //DEBUG
		std::ostringstream fn;
		fn << "m-" << size.x << "x" << size.y << "-after.png";
		write_solution(fn.str(), starts, soln);
	}
	*/

}

int launch_solve(void *_data) {
	SolverData *data = (SolverData *)_data;

	{
		vector< size_t > starts;
		vector< Real > soln;
		if (data->p->constraints.empty()) {
			LOG_INFO("Bailing on solution -- no constraints.");
			make_starts(*(data->p), starts);
			LOG_INFO("Have " << starts.size() << " starts.");
			if (starts.size()) {
				LOG_INFO("Have " << starts.back() << " variables.");
				soln.resize(starts.back(), Real(0.0));
			}
		} else {
			Timer timer;
			multiscale_solve(data, *(data->p), starts, soln, timer);
		}
		LOG_INFO("Calling write_solution...");
		write_solution(data, *(data->p), "out.png", starts, soln);
		data->did_end = true;
	}

	return 0;
}
