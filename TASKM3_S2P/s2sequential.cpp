#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <ctime>

using namespace std;
using namespace std::chrono;

void randomVector(int* vector, int size) {
    for (int i = 0; i < size; i++) {
        vector[i] = rand() % 100;
    }
}

int main(int argc, char* argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);                          // Initialize MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);            // Get current process ID
    MPI_Comm_size(MPI_COMM_WORLD, &size);            // Get number of processes

    unsigned long N = 10000;
    int* v1 = nullptr;
    int* v2 = nullptr;
    int* v3 = nullptr;

    int chunk = N / size;

    int* local_v1 = new int[chunk];
    int* local_v2 = new int[chunk];
    int* local_v3 = new int[chunk];

    srand(time(0));

    auto start = high_resolution_clock::now();

    if (rank == 0) {
        v1 = new int[N];
        v2 = new int[N];
        v3 = new int[N];
        randomVector(v1, N);
        randomVector(v2, N);
    }

    // Scatter the data to all processes
    MPI_Scatter(v1, chunk, MPI_INT, local_v1, chunk, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(v2, chunk, MPI_INT, local_v2, chunk, MPI_INT, 0, MPI_COMM_WORLD);

    // Each process adds its chunk
    for (int i = 0; i < chunk; i++) {
        local_v3[i] = local_v1[i] + local_v2[i];
    }

    // Gather the results back to root
    MPI_Gather(local_v3, chunk, MPI_INT, v3, chunk, MPI_INT, 0, MPI_COMM_WORLD);

    // Reduce to find the total sum of v3
    int local_sum = 0, total_sum = 0;
    for (int i = 0; i < chunk; i++) {
        local_sum += local_v3[i];
    }

    MPI_Reduce(&local_sum, &total_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    auto stop = high_resolution_clock::now();

    if (rank == 0) {
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by MPI function: " << duration.count() << " microseconds" << endl;
        cout << "Total sum of v3: " << total_sum << endl;

        delete[] v1;
        delete[] v2;
        delete[] v3;
    }

    delete[] local_v1;
    delete[] local_v2;
    delete[] local_v3;

    MPI_Finalize();
    return 0;
}
