#include <mpi.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int rank, size;
    char message[100];

    MPI_Init(&argc, &argv);                     
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);       
    MPI_Comm_size(MPI_COMM_WORLD, &size);       

    if (rank == 0) {  
        strcpy(message, "Hello World!");
        for (int i = 1; i < size; i++) {
            MPI_Send(message, strlen(message) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
    } else { 
        MPI_Recv(message, 100, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d received message: %s\n", rank, message);
    }

    MPI_Finalize(); /
    return 0;
}
