#include <iostream>

//NOTE: matrix in COLUMN-MAJOR order!!!
template< typename TYPE >
void col_dump(const TYPE * col_mat, unsigned int rows, unsigned int cols) {
	for (unsigned int r = 0; r < rows; ++r) {
		if (r == 0) std::cout << '[';
		else std::cout << ' ';
		for (unsigned int c = 0; c < cols; ++c) {
			if (c) std::cout << ' ';
			std::cout << col_mat[r + c * rows];
		}
		if (r + 1 == rows) std::cout << "]" << std::endl;
		else std::cout << ";...\n";
	}
}

//and row-major version:
template< typename TYPE >
void row_dump(const TYPE * row_mat, unsigned int rows, unsigned int cols) {
	for (unsigned int r = 0; r < rows; ++r) {
		if (r == 0) std::cout << '[';
		else std::cout << ' ';
		for (unsigned int c = 0; c < cols; ++c) {
			if (c) std::cout << ' ';
			std::cout << row_mat[r * cols + c];
		}
		if (r + 1 == rows) std::cout << "]" << std::endl;
		else std::cout << ";...\n";
	}
}
