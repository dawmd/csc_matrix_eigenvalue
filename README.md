# Sparse matrix library
The library is small and offers the following functionalities:
* Vector API
    * basic arithmetic operations: addition, subtraction, multiplying by a scalar
    * computing the Euclidean norm
    * normalizing a vector
* Sparse matrix in the CSC format API
    * multiplying a matrix by a vector
    * checking if a matrix is upper-triangular
    * solving a system of linear equations for upper-triangular matrices
    * finding the maximum eigenvalue of a matrix up to the absolute value

Along with the library, I provide a basic logging header as well as tests. They have not been finished yet and the bash script is not very helpful; nonetheless, it provides the user with *some* way of confirming the correctness of the implemneted algorithms.

## Building
Modify the `CMakeLists.txt` file provided with the library to adjust it to your needs. Afterwards, you can build it by
```sh
mdkir build
cd build
cmake ..
make
```
which will generate an executable in the `build` subdirectory.

## Running tests
After building the executable, go to the `tests` directory and, if the tests have not been generated yet, adjust the Python script `generate.py` and then run it:
```sh
python3 generate.py
```
That should generate tests in the `tests` directory. When it's done, run the testing bash script:
```sh
chmod u+x run_tests.sh
./run_tests.sh
```
