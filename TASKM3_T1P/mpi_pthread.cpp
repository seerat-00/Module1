#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <pthread.h>

#define N 100
#define MAX_THREADS 4

int A[N][N], B[N][N], C[N][N];

int rows_per_proc;
int sub_A[N][N], sub_C[N][N];
int rank;

struct ThreadData {
    int start_row;
    int end_row;
};

void* multiply_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < N; j++) {
            sub_C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                sub_C[i][j] += sub_A[i][k] * B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

void fill_matrix() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
}

int main(int argc, char** argv) {
    int size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    rows_per_proc = N / size;

    if (rank == 0) {
        srand(time(NULL));
        fill_matrix();
    }

    MPI_Bcast(B, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(A, rows_per_proc * N, MPI_INT, sub_A, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];

    double start = MPI_Wtime();

    for (int t = 0; t < MAX_THREADS; t++) {
        thread_data[t].start_row = t * (rows_per_proc / MAX_THREADS);
        thread_data[t].end_row = (t + 1) * (rows_per_proc / MAX_THREADS);
        pthread_create(&threads[t], NULL, multiply_thread, (void*)&thread_data[t]);
    }

    for (int t = 0; t < MAX_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    double end = MPI_Wtime();

    MPI_Gather(sub_C, rows_per_proc * N, MPI_INT, C, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "[MPI + Pthreads] execution Time - Multithreading: " << (end - start) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

// To compile: mpic++ -pthread matrix_mpi.cpp -o matrix_mpi_pthread
// To run: mpirun -np 4 ./matrix_mpi_pthread
