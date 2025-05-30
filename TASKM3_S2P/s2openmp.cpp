#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int VECTOR_SIZE = 500000; 

void randomVector(int* vec, int size) {
    for (int i = 0; i < size; i++) {
        vec[i] = rand() % 100;
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int chunk_size = VECTOR_SIZE / size;
    int* v1 = nullptr;
    int* v2 = nullptr;
    int* v3_local = new int[chunk_size];

    int* v1_local = new int[chunk_size];
    int* v2_local = new int[chunk_size];

    if (rank == 0) {
        v1 = new int[VECTOR_SIZE];
        v2 = new int[VECTOR_SIZE];
        randomVector(v1, VECTOR_SIZE);
        randomVector(v2, VECTOR_SIZE);
    }

    MPI_Scatter(v1, chunk_size, MPI_INT, v1_local, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(v2, chunk_size, MPI_INT, v2_local, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    auto start = high_resolution_clock::now();

    long long local_sum = 0;

    #pragma omp parallel for reduction(+:local_sum)
    for (int i = 0; i < chunk_size; i++) {
        v3_local[i] = v1_local[i] + v2_local[i];
        local_sum += v3_local[i];
    }

    long long total_sum = 0;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    auto stop = high_resolution_clock::now();

    if (rank == 0) {
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Total sum: " << total_sum << endl;
        cout << "Time taken: " << duration.count() << " microseconds" << endl;
        delete[] v1;
        delete[] v2;
    }

    delete[] v1_local;
    delete[] v2_local;
    delete[] v3_local;

    MPI_Finalize();
    return 0;
}
