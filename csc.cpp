
#include <matrix.h>
#include <iostream>

using namespace csc;

int main() {
    float vals[] = { -50.3f, 20.111f, 20.0f, -40.0f };
    std::size_t idxs[] = { 0, 2, 1, 3 };
    std::size_t cc[] = { 0, 1, 2, 3, 4 };

    csc_mat<float, 4, 4> mat(vals, idxs, cc);
    std::cout << mat.find_max_eigenvalue(1e-8) << "\n";
}