#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>

#define N 1000

int A[N][N], B[N][N], C[N][N];

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

    int rows = N / size;
    int sub_A[rows][N], sub_C[rows][N];

    if (rank == 0) {
        srand(time(NULL));
        fill_matrix();
    }

    MPI_Bcast(B, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(A, rows * N, MPI_INT, sub_A, rows * N, MPI_INT, 0, MPI_COMM_WORLD);

    double start = MPI_Wtime();
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < N; j++) {
            sub_C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                sub_C[i][j] += sub_A[i][k] * B[k][j];
            }
        }
    }
    double end = MPI_Wtime();

    MPI_Gather(sub_C, rows * N, MPI_INT, C, rows * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "[MPI] Time for execution (sequential): " << (end - start) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

// To compile: mpic++ matrix_mpi.cpp -o matrix_mpi
// To run (example with 4 processes): mpirun -np 4 ./matrix_mpi
