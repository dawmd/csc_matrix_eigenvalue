import numpy as np
import scipy as sp

iterations = 10

for size_exp in range(iterations):
    size = np.random.randint(1, 2 ** (4 + size_exp))

    for scalar_exp in range(10):
        matrix = float(2 ** scalar_exp) * sp.sparse.rand(size, size, density=0.25)
        matrix = matrix + matrix.T # Real, symmetric matrices have real eigenvalues
        matrix = matrix.tocsc()

        eigs = np.abs(np.real(sp.linalg.eigvals(matrix.toarray())))
        # min_eig = min(eigs)
        max_eig = max(eigs)

        with open("test" + str(size_exp) + "_" + str(scalar_exp) + ".in", "w") as f:
            print(matrix.data.size, file=f)
            for value in matrix.data:
                print('{:f}'.format(value), end=" ", file=f)
            print('', file=f)
            for row in matrix.indices:
                print(row, end=" ", file=f)
            print('', file=f)
            print(size + 1, file=f)
            for col_count in matrix.indptr:
                print(col_count, end=" ", file=f)
            print('', file=f)

        with open("test" + str(size_exp) + "_" + str(scalar_exp) + ".out", "w") as f:
            # print(min_eig)
            print(max_eig, file=f)
