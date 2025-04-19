#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <omp.h>

#define N 100

int A[N][N], B[N][N], C[N][N];
int sub_A[N][N], sub_C[N][N];

void fill_matrix() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
}

int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows_per_proc = N / size;

    if (rank == 0) {
        srand(time(NULL));
        fill_matrix();
    }

    MPI_Bcast(B, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(A, rows_per_proc * N, MPI_INT, sub_A, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    double start = MPI_Wtime();

    #pragma omp parallel for
    for (int i = 0; i < rows_per_proc; i++) {
        for (int j = 0; j < N; j++) {
            sub_C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                sub_C[i][j] += sub_A[i][k] * B[k][j];
            }
        }
    }

    double end = MPI_Wtime();

    MPI_Gather(sub_C, rows_per_proc * N, MPI_INT, C, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "[MPI + OpenMP] ececution Time - multithreading: " << (end - start) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

// To compile: mpic++ -fopenmp matrix_mpi_openmp.cpp -o matrix_mpi_openmp
// To run: mpirun -np 4 ./matrix_mpi_openmp
