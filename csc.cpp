
#include <matrix.h>
#include <iostream>

#include <vector>

using namespace csc;

int main() {
    using T = double;

    std::size_t value_count;
    std::cin >> value_count;

    std::vector<T> values(value_count);
    for (std::size_t i = 0; i < value_count; ++i) {
        std::cin >> values[i];
    }

    std::vector<std::size_t> rows(value_count);
    for (std::size_t i = 0; i < value_count; ++i) {
        std::cin >> rows[i];
    }

    std::size_t matrix_size;
    std::cin >> matrix_size;

    std::vector<std::size_t> col_count(matrix_size);
    for (std::size_t i = 0; i < matrix_size; ++i) {
        std::cin >> col_count[i];
    }

    --matrix_size;

    csc_mat<T> mat(values, rows, col_count, matrix_size, matrix_size);
    
    std::cout << mat.find_max_eigenvalue(1e-6, false) << '\n';
}