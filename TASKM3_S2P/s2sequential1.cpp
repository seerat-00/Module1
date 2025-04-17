#include <chrono>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <mpi.h>  

using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int size) {
  for (int i = 0; i < size; i++) {
    vector[i] = rand() % 100;
  }
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);  
  unsigned long size = 1000000;  
  srand(time(0));

  int rank, num_procs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);  
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);  

  int *v1, *v2, *v3;
  int local_sum = 0, global_sum = 0;
  auto start = high_resolution_clock::now();

  v1 = (int*)malloc(size * sizeof(int));
  v2 = (int*)malloc(size * sizeof(int));
  v3 = (int*)malloc(size * sizeof(int));

  randomVector(v1, size);
  randomVector(v2, size);

  for (int i = 0; i < size; i++) {
    v3[i] = v1[i] + v2[i];
    local_sum += v3[i];  
  }
  
  MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Total sum of v3: " << global_sum << endl;
    cout << "Time taken: " << duration.count() << " microseconds" << endl;
  }
  
  MPI_Finalize();
  return 0;
}
