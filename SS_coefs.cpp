#include "SS_coefs.hpp"
#include "SS_nice_dump.hpp"

#include <mkl_lapack.h>
#include <mkl_blas.h>

#include <limits>
#include <utility>
#include <iostream>
#include <cmath>

using std::make_pair;
using std::swap;
using std::vector;
using std::cout;
using std::endl;

//TwoSet == pair of elements, with first element less than second.
typedef uint32_t TwoSetIndex;
typedef std::pair< LayerIndex, LayerIndex > TwoSet;

TwoSet to_twoset(TwoSetIndex index) {
	TwoSet ret = make_pair(0,1);
	while (index >= ret.second) {
		index -= ret.second;
		ret.second += 1;
	}
	ret.first = index;
	return ret;
}

TwoSetIndex to_twoset_index(TwoSet const &in) {
	assert(in.first < in.second);
	return in.second * (in.second - 1) / 2 + in.first;
}

TwoSetIndex count_twosets(unsigned int layers) {
	return to_twoset_index(make_pair(0,layers));
}


StackingIndex to_stacking_index(Stacking const &in) {
	vector< unsigned int > num(in.size()); //what is at a given location
	vector< unsigned int > pos(in.size()); //where is a given number
	for (unsigned int i = 0; i < in.size(); ++i) {
		num[i] = i;
		pos[i] = i;
	}
	StackingIndex ret = 0;
	StackingIndex fac = 1;
	for (unsigned int i = 0; i < in.size(); ++i) {
		assert(num[pos[in[i]]] == in[i]);
		unsigned int s = pos[in[i]]; //will swap with where in[i] is at the moment.

		assert(s >= i);
		ret += fac * (s - i);
		fac *= (in.size() - i);
		if (s != i) {
			swap(num[s], num[i]);
			pos[num[s]] = s;
			pos[num[i]] = i;
		}
	}
	return ret;
}

Stacking to_stacking(StackingIndex index, unsigned int layers) {
	Stacking stacking(layers);
	for (unsigned int i = 0; i < layers; ++i) {
		stacking[i] = i;
	}
	for (unsigned int i = 0; i < layers; ++i) {
		swap(stacking[i], stacking[i + (index % (layers - i))]);
		index /= (layers - i);
	}
	return stacking;
}

size_t count_stackings(unsigned int layers) {
	static vector< StackingIndex > factorials(2,1);
	while (factorials.size() <= layers) {
		factorials.push_back(factorials.back() * factorials.size());
	}
	return factorials[layers];
}

DeltaIndex to_delta_index(Delta const &in) {
	assert(in.pos < in.base.size() && in.pos + 1 < in.base.size());
	assert(in.base[in.pos] < in.base[in.pos + 1]);
	Stacking temp = in.base;
	temp.erase(temp.begin() + in.pos, temp.begin() + in.pos + 2);
	for (unsigned int i = 0; i < temp.size(); ++i) {
		if (temp[i] > in.base[in.pos + 1]) temp[i] -= 1;
		if (temp[i] > in.base[in.pos]) temp[i] -= 1;
	}
	return to_twoset_index(make_pair(in.base[in.pos], in.base[in.pos+1])) + count_twosets(in.base.size()) * (in.pos + (in.base.size()-1) * to_stacking_index(temp));
}

Delta to_delta(DeltaIndex index, unsigned int layers) {
	assert(layers >= 2);
	TwoSet elts = to_twoset(index % count_twosets(layers));
	index /= count_twosets(layers);
	unsigned int pos = index % (layers - 1);
	index /= (layers - 1);
	Stacking temp = to_stacking(index, layers-2);
	assert(elts.first < elts.second);
	for (unsigned int i = 0; i < temp.size(); ++i) {
		if (temp[i] >= elts.first) temp[i] += 1;
		if (temp[i] >= elts.second) temp[i] += 1;
	}
	assert(sizeof(TwoSet) == 2 * sizeof(LayerIndex));
	temp.insert(temp.begin() + pos, &elts.first, &elts.first + 2);
	return Delta(temp, pos);
}

size_t count_deltas(unsigned int layers) {
	return count_stackings(layers) * (layers - 1) / 2;
}

void pinv_and_null(const float *matrix, unsigned int rows, unsigned int cols, vector< float > &pinv, vector< float > &null) {
	pinv.clear();
	null.clear();
	if (rows == 0 || cols == 0) return;
	unsigned int min_rc = rows;
	if (cols < min_rc) min_rc = cols;
	vector< float > col_matrix(rows * cols);
	for (unsigned int row = 0; row < rows; ++row) {
		for (unsigned int col = 0; col < cols; ++col) {
			col_matrix[col * rows + row] = matrix[row * cols + col];
		}
	}
	//col_dump(&col_matrix[0], rows, cols);
	vector< float > col_u(rows * min_rc); //skip last few columns
	vector< float > s(min_rc);
	vector< float > col_vt(cols * cols); //need full matrix for null space
//Note: have to deal with fortran column-major arrays?
	{ //Perform SVD:
		char jobu = 'S';
		char jobvt = 'A';
		MKL_INT m = rows;
		MKL_INT n = cols;
		/* float *a = matrix in */;
		MKL_INT lda = m; //"leading dimension" of a
		/* float *s = output singular values -- length min(m,n) */;
		/* float *u = output left matrix, m*m */;
		MKL_INT ldu = m;
		/* float *vt = output right matrix^t, n*n */;
		MKL_INT ldvt = n;
		/* float *work need to figure it out by querying with lwork == -1, then supply*/;
		MKL_INT lwork = -1/* size of work array */;
		MKL_INT info = 0/* output: did the routine converge? */;

		float work_est = 0;
		//Call to get estimate of work array size:
		sgesvd(&jobu, &jobvt, &m, &n, &col_matrix[0], &lda, &s[0], &col_u[0], &ldu, &col_vt[0], &ldvt, &work_est, &lwork, &info);
		if (info != 0) {
			cout << "Info of '" << info << "' when computing work estimate!" << endl;
			exit(1);
		}
		//cout << "For " << m << " x " << n << ", need " << work_est << " storage." << endl;
		vector< float > work((int)work_est + 1);
		//Call to actually compute svd:
		lwork = work.size();
		sgesvd(&jobu, &jobvt, &m, &n, &col_matrix[0], &lda, &s[0], &col_u[0], &ldu, &col_vt[0], &ldvt, &work[0], &lwork, &info);
		if (info != 0) {
			cout << "Info of '" << info << "' when computing SVD!" << endl;
			exit(1);
		}
	}
	/*
	cout << "U:" << endl;
	col_dump(&col_u[0], rows, min_rc);
	cout << "S:" << endl;
	col_dump(&s[0], 1, min_rc);
	cout << "V^t:" << endl;
	col_dump(&col_vt[0], cols, cols);
	*/
	
	//compute tolerance for an element to be considered nonzero;
	//using tolerance wikipedia reports for matlab.
	unsigned int keep = 0;
	{
		float tol = std::numeric_limits< float >::epsilon() * s[0];
		if (rows > cols) tol *= rows;
		else tol *= cols;
		while (keep < s.size() && s[keep] > tol) ++keep;
	}

	//rows [keep .. cols] of vt are the null space:
	for (unsigned int c = 0; c < cols; ++c) {
		for (unsigned int r = keep; r < cols; ++r) {
			null.push_back(col_vt[c * cols + r]);
		}
	}

	//Compute pinv() as vt' * pinv(s) * u':
	//pinv(s) is simply invert non-zeros:
	for (unsigned int i = 0; i < keep; ++i) {
		s[i] = 1.f / s[i];
	}
	for (unsigned int i = keep; i < s.size(); ++i) {
		s[i] = 0.f;
	}
	//since s is zero everywhere but for a keep x keep block, we can
	//perform this as a ( cols x keep ) * ( keep x keep ) * ( keep x rows )
	//operation.
	
	//multiply pinv(s) * u':
	for (unsigned int c = 0; c < keep; ++c) {
		//s[c] multiplies row c of u' == col c of u
		for (unsigned int i = 0; i < rows; ++i) {
			col_u[c * rows + i] *= s[c];
		}
	}
	//multiply vt' * u':
	//We only care about cols x keep block of vt', keep x rows block of u'
	// (== keep x cols of vt, rows x keep of u)
	vector< float > col_pinv(rows * cols);
	{
		char transa = 'T';
		char transb = 'T';
		MKL_INT m = cols;
		MKL_INT n = rows;
		MKL_INT k = keep;
		MKL_INT ldvt = cols;
		MKL_INT ldu = rows;
		float alpha = 1.f;
		float beta = 0.f;
		MKL_INT ldpinv = cols;
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &col_vt[0], &ldvt, &col_u[0], &ldu,
		      &beta, &col_pinv[0], &ldpinv);
	}
	//transpose pinv...
	pinv.resize(cols * rows);
	//Note pinv is cols x rows, so this is a bit odd looking:
	for (unsigned int r = 0; r < rows; ++r) {
		for (unsigned int c = 0; c < cols; ++c) {
			pinv[c * rows + r] = col_pinv[r * cols + c];
		}
	}
}

void build_factors_matrix(std::vector< float > const &alphas, std::vector< float > &into) {
	/*cout << "Alphas:\n";
	row_dump(&alphas[0], 1, alphas.size());
	*/
	unsigned int layers = alphas.size();
	unsigned int deltas = count_deltas(layers);
	unsigned int stackings = count_stackings(layers);

	assert(alphas.size() >= 2);

	into.clear();
	if (alphas.empty()) {
		return;
	}

	vector< float > deltas_to_diffs(stackings * deltas, 0.f);
	vector< float > weights(deltas, 0.f);
	for (DeltaIndex delta_index = 0; delta_index < deltas; ++delta_index) {
		Delta delta = to_delta(delta_index, layers);
		StackingIndex s1 = to_stacking_index(delta.base);
		swap(delta.base[delta.pos], delta.base[delta.pos+1]);
		StackingIndex s2 = to_stacking_index(delta.base);
		deltas_to_diffs[s1 * deltas + delta_index] = 1.f;
		deltas_to_diffs[s2 * deltas + delta_index] =-1.f;
		//weights are inside square if no sqrtf:
		weights[delta_index] = alphas[delta.base[delta.pos]] * alphas[delta.base[delta.pos+1]];
	}

	/*
	cout << "Deltas to diffs:\n";
	row_dump(&deltas_to_diffs[0], stackings, deltas);
	cout << "Weights:\n";
	row_dump(&weights[0], 1, deltas);
	*/


	vector< float > pinv_deltas_to_diffs;
	vector< float > null_deltas_to_diffs;
	pinv_and_null(&deltas_to_diffs[0], stackings, deltas, pinv_deltas_to_diffs, null_deltas_to_diffs);
	unsigned int null_cols = null_deltas_to_diffs.size() / deltas;

	if (null_cols == 0) {
		for (unsigned int r = 0; r < deltas; ++r) {
			for (unsigned int c = 0; c < stackings; ++c) {
				pinv_deltas_to_diffs[r * stackings + c] *= weights[r];
			}
		}
		into.resize(stackings * stackings);
		{
			char transa = 'N';
			char transb = 'T';
			MKL_INT m = stackings;
			MKL_INT n = stackings;
			MKL_INT k = deltas;
			MKL_INT lda = stackings;
			MKL_INT ldb = stackings;
			float alpha = 2.f;
			float beta = 0.f;
			MKL_INT ldc = stackings;
			sgemm(&transa, &transb, &m, &n, &k,
			      &alpha, &pinv_deltas_to_diffs[0], &lda, &pinv_deltas_to_diffs[0], &ldb,
			      &beta, &into[0], &ldc);
		}
		return;
	}

	vector< float > weight_x_null = null_deltas_to_diffs;
	{ //weight x null => multiply each row of null by corresponding weight.
		assert(weight_x_null.size() == deltas * null_cols);
		for (unsigned int r = 0; r < deltas; ++r) {
			for (unsigned int c = 0; c < null_cols; ++c) {
				weight_x_null[r * null_cols + c] *= weights[r];
			}
		}
	}

	/*
	cout << "Weights x null:\n";
	row_dump(&weight_x_null[0], deltas, null_cols);
	*/

	//we need the pseudoinverse of weight x null, but not the null space:
	vector< float > pinv_weight_x_null;
	{
		vector< float > null_trash;
		pinv_and_null(&weight_x_null[0], deltas, null_cols, pinv_weight_x_null, null_trash);
	}


	//Want I - weight_x_null * pinv_weight_x_null:
	// == (pinv_weight_x_null' * weight_x_null')'
	vector< float > i_minus_etc(deltas * deltas, 0.f);
	for (unsigned int i = 0; i < deltas; ++i) {
		i_minus_etc[i * deltas + i] = 1.f;
	}
	{
		assert(pinv_weight_x_null.size() == deltas * null_cols);
		assert(weight_x_null.size() == deltas * null_cols);
		char transa = 'N';
		char transb = 'N';
		MKL_INT m = deltas;
		MKL_INT n = deltas;
		MKL_INT k = null_cols;
		MKL_INT lda = deltas;
		MKL_INT ldb = null_cols;
		float alpha =-1.f;
		float beta = 1.f;
		MKL_INT ldc = deltas;
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &pinv_weight_x_null[0], &lda, &weight_x_null[0], &ldb,
		      &beta, &i_minus_etc[0], &ldc);
	}

	/*
	cout << "I - ...\n";
	row_dump(&i_minus_etc[0], deltas, deltas);
	*/

	/*
	cout << "pinv:\n";
	row_dump(&pinv_deltas_to_diffs[0], stackings, deltas);
	*/

	//Want I_minus_etc * w * pinv_delta_to_diffs.
	//first multiply by weights:
	assert(pinv_deltas_to_diffs.size() == deltas * stackings);
	for (unsigned int r = 0; r < deltas; ++r) {
		for (unsigned int c = 0; c < stackings; ++c) {
			pinv_deltas_to_diffs[r * stackings + c] *= weights[r];
		}
	}
	/*
	cout << "weights * pinv:\n";
	row_dump(&pinv_deltas_to_diffs[0], stackings, deltas);
	*/

	vector< float > stackings_to_weighted_deltas(deltas * stackings);
	{
		assert(pinv_deltas_to_diffs.size() == deltas * stackings);
		assert(i_minus_etc.size() == deltas * deltas);
		char transa = 'N';
		char transb = 'N';
		MKL_INT m = stackings;
		MKL_INT n = deltas;
		MKL_INT k = deltas;
		MKL_INT lda = stackings;
		MKL_INT ldb = deltas;
		float alpha = 1.f;
		float beta = 0.f;
		MKL_INT ldc = stackings;
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &pinv_deltas_to_diffs[0], &lda, &i_minus_etc[0], &ldb,
		      &beta, &stackings_to_weighted_deltas[0], &ldc);
	}

	//partials == 2 * stackings_to_weighted_deltas' * stackings_to_weighted_deltas
	into.resize(stackings * stackings);
	{
		assert(stackings_to_weighted_deltas.size() == deltas * stackings);
		char transa = 'N';
		char transb = 'T';
		MKL_INT m = stackings;
		MKL_INT n = stackings;
		MKL_INT k = deltas;
		MKL_INT lda = stackings;
		MKL_INT ldb = stackings;
		float alpha = 2.f;
		float beta = 0.f;
		MKL_INT ldc = stackings;
		sgemm(&transa, &transb, &m, &n, &k,
		      &alpha, &stackings_to_weighted_deltas[0], &lda, &stackings_to_weighted_deltas[0], &ldb,
		      &beta, &into[0], &ldc);
	}

	/*cout << "partials:\n";
	row_dump(&into[0], stackings, stackings);
	*/
}

void test_indexing() {
	{ //test twoset indexing... well, the first 100:
		cout << "Checking twoset indexing..."; cout.flush();
		TwoSet old = make_pair(0,0);
		for (unsigned int i = 0; i < 100; ++i) {
			TwoSet pair = to_twoset(i);
			assert(pair != old);
			assert(to_twoset_index(pair) == i);
			old = pair;
		}
		cout << " done." << endl;
	}

	{ //check stacking_index() / stacking()
		cout << "Checking stacking indexing..."; cout.flush();
		for (unsigned int layers = 1; layers < 5; ++layers) {
			Stacking stack(layers);
			for (unsigned int i = 0; i < layers; ++i) {
				stack[i] = i;
			}
			vector< Stacking > stackings;
			unsigned int i = 0;
			do {
				/*
				//DEBUG:
				for (unsigned int p = 0; p < stack.size(); ++p) {
					cout << stack[p] << ' ';
				}
				cout << "=> " << to_stacking_index(stack) << endl;
				Stacking out = to_stacking(to_stacking_index(stack), layers);
				cout << "=>";
				for (unsigned int p = 0; p < out.size(); ++p) {
					cout << ' ' << out[p];
				}
				cout << endl;
				*/

				assert(to_stacking(to_stacking_index(stack), layers) == stack);
				++i;
				stackings.push_back(stack);
			} while (next_permutation(stack.begin(), stack.end()));
			assert(stackings.size() == count_stackings(layers));
		}
		cout << " done." << endl;
	}

	{ //check delta_index() / delta()
		cout << "Checking delta indexing..."; cout.flush();
		for (unsigned int layers = 1; layers < 5; ++layers) {
			vector< unsigned int > stack_sums(count_stackings(layers), 0);
			for (DeltaIndex idx = 0; idx < count_deltas(layers); ++idx) {
				Delta del = to_delta(idx, layers);
				assert(to_delta_index(del) == idx);
				assert(del.base.size() == layers);
				assert(del.pos + 1 < layers && del.pos < layers);
				StackingIndex s_ind = to_stacking_index(del.base);
				std::swap(del.base[del.pos], del.base[del.pos+1]);
				StackingIndex s_ind2 = to_stacking_index(del.base);
				assert(s_ind < stack_sums.size());
				assert(s_ind2 < stack_sums.size());
				assert(s_ind != s_ind2);
				stack_sums[s_ind] += 1;
				stack_sums[s_ind2] += 1;
			}
			for (unsigned int i = 0; i < stack_sums.size(); ++i) {
				assert(stack_sums[i] == layers-1);
			}
		}
		cout << " done." << endl;
	}

}


void svd_subtest(float const *mat, unsigned int rows, unsigned int cols, float const *pinv, float const *null, unsigned int null_cols) {
	vector< float > test_pinv, test_null;
	pinv_and_null(mat, rows, cols, test_pinv, test_null);
	assert(test_pinv.size() == rows * cols);
	assert(test_null.size() == cols * null_cols);
	/*cout << "Got pinv:" << endl;
	row_dump(&test_pinv[0], cols, rows);
	cout << "Expected:" << endl;
	row_dump(pinv, cols, rows);*/
	for (unsigned int i = 0; i < test_pinv.size(); ++i) {
		assert(fabs(test_pinv[i] - pinv[i]) < 0.001f * fabs(pinv[i]));
	}
	if (null_cols) {
	/*
		cout << "Got null:" << endl;
		row_dump(&test_null[0], cols, null_cols);
		cout << "Expected:" << endl;
		row_dump(null, cols, null_cols);
		*/
	}
	for (unsigned int c = 0; c < null_cols; ++c) {
		bool good_null = false;
		for (int sign = -1; sign <= 1; sign += 2) {
			bool fail = false;
			for (unsigned int r = 0; r < cols; ++r) {
				float test_val = test_null[r * null_cols + c] * sign;
				float val = null[r * null_cols + c];
				if (fabs(test_val - val) >= 0.001f * fabs(val) + 0.001f) {
					fail = true;
					break;
				}
			}
			good_null = good_null || !fail;
		}
		assert(good_null);
	}
}

void test_svd() {
	cout << "Testing pinv_and_null computation..."; cout.flush();
	//[ 0 1 2 ; 2 3 1 ; 5 5 6 ; 2 2 1 ]
	{
		const unsigned int rows = 4, cols = 3;
		float mat[rows * cols] = {
			0.f, 1.f, 2.f,
			2.f, 3.f, 1.f,
			5.f, 5.f, 6.f,
			2.f, 2.f, 1.f };
		float pinv[cols * rows] = {
			-0.679901f, -0.320099f, 0.245658f, 0.205955,
			 0.416873f,  0.583127f,-0.238213f, 0.012407,
			 0.230769f, -0.230769f, 0.153846f,-0.153846
		};
		svd_subtest(mat, rows, cols, pinv, NULL, 0);
	}
	//[ 2 1 1; -1 0 3]
	{
		const unsigned int rows = 2, cols = 3;
		float mat[rows * cols] = {
			2.f, 1.f, 1.f,
			-1.f, 0.f, 3.f };
		float pinv[cols * rows] = {
			0.355932f, -0.135593f,
			0.169492f, -0.016949f,
			0.118644f,  0.288136f };
		const unsigned int null_cols = 1;
		float null[cols * null_cols] = {
			0.39057f,
			-0.91132f,
			0.13019f };
		svd_subtest(mat, rows, cols, pinv, null, null_cols);
	}
	//[ 1 1 1 1 ; 2 2 2 2 ; 1 2 2 2 ; 3 3 3 3 ]
	{
		const unsigned int rows = 4, cols = 4;
		float mat[rows * cols] = {
			1.f, 1.f, 1.f, 1.f,
			2.f, 2.f, 2.f, 2.f,
			1.f, 2.f, 2.f, 2.f,
			3.f, 3.f, 3.f, 3.f
			};
		float pinv[cols * rows] = {
			 0.142857f,  0.285714f, -1.000000f,  0.428571f,
			-0.023810f, -0.047619f,  0.333333f, -0.071429f,
			-0.023810f, -0.047619f,  0.333333f, -0.071429f,
			-0.023810f, -0.047619f,  0.333333f, -0.071429f,
			};
		const unsigned int null_cols = 2;
		float null[cols * null_cols] = {
			0.00000f,  0.00000f,
			-0.81650f,  0.00000f,
			0.40825f, -0.70711f,
			0.40825f,  0.70711f,
			};
		svd_subtest(mat, rows, cols, pinv, null, null_cols);
	}

	//[ 1 0 1 0 0 ; 0 1 0 1 1 ; 0 0 1 0 -2 ]
	{
		const unsigned int rows = 3, cols = 5;
		float mat[rows * cols] = {
			1.f, 0.f, 1.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 1.f, 1.f,
			0.f, 0.f, 1.f, 0.f,-2.f,
			};
		float pinv[cols * rows] = {
			0.578947f, -0.105263f, -0.157895f,
			-0.105263f,  0.473684f,  0.210526f,
			0.421053f,  0.105263f,  0.157895f,
			-0.105263f,  0.473684f,  0.210526f,
			0.210526f,  0.052632f, -0.421053f,
			};
		const unsigned int null_cols = 2;
		float null[cols * null_cols] = {
			0.167944f, -0.626775f,
			-0.641027f, -0.339707f,
			-0.167944f,  0.626775f,
			0.724999f,  0.026319f,
			-0.083972f,  0.313388f,
			};
		svd_subtest(mat, rows, cols, pinv, null, null_cols);
	}
	cout << "done."; cout.flush();

	/*
	float
	cout << "Performing test pinv/null of:" << endl;
	row_dump(test_mat, m, n);
	vector< float > pinv, null;
	pinv_and_null(test_mat, m, n, pinv, null);
	cout << "null:" << endl;
	assert(null.size() % n == 0);
	row_dump(&null[0], n, null.size() / n);
	cout << "pinv:" << endl;
	assert(pinv.size() == n * m);
	row_dump(&pinv[0], n, m);
	*/

	vector< float > factors;
	std::vector< float > alphas;
	alphas.push_back(255.f / 255.f);
	alphas.push_back(1.f / 255.f);
	alphas.push_back(100.f / 255.f);
	build_factors_matrix(alphas, factors);
	cout << "Factors (" << alphas.size() << "):\n";
	row_dump(&factors[0], count_stackings(alphas.size()), count_stackings(alphas.size()));

}
