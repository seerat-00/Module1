#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono> 

using namespace std;
using namespace chrono; 

int partition(vector<int>& arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for(int j = low; j < high; j++) {
        if(arr[j] <= pivot) {
            i++;
            swap(arr[i], arr[j]);
        }
    }

    swap(arr[i + 1], arr[high]);
    return i + 1;
}

void quickSort(vector<int>& arr, int low, int high) {
    if(low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    vector<int> data;
    int n;

    steady_clock::time_point start, end; 

    if(rank == 0) {
        n = 10000; 
        data.resize(n);
        for(int i = 0; i < n; i++) {
            data[i] = rand() % 1000000;
        }

        start = steady_clock::now();
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int chunk_size = n / size;
    vector<int> local_data(chunk_size);

    MPI_Scatter(data.data(), chunk_size, MPI_INT,
                local_data.data(), chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    quickSort(local_data, 0, chunk_size - 1);

    vector<int> sorted_data;
    if(rank == 0) sorted_data.resize(n);

    MPI_Gather(local_data.data(), chunk_size, MPI_INT,
               sorted_data.data(), chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank == 0) {
        sort(sorted_data.begin(), sorted_data.end()); 
        end = steady_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        cout << "First 10 sorted values: ";
        for(int i = 0; i < 10; ++i) cout << sorted_data[i] << " ";
        cout << "\nTime take : " << duration.count() << " ms" << endl;
    }

    MPI_Finalize();
    return 0;
}
