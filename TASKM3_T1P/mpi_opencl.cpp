// matrix_mpi_opencl.cpp - MPI + OpenCL Matrix Multiplication
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <CL/cl.h>

const int N = 1000;

int A[N][N], B[N][N], C[N][N];
int sub_A[N][N], sub_C[N][N];

const char* kernelSource = R"(
__kernel void mat_mul(
    __global int* A,
    __global int* B,
    __global int* C,
    int N,
    int rows_per_proc
) {
    int row = get_global_id(0);
    for (int j = 0; j < N; j++) {
        int sum = 0;
        for (int k = 0; k < N; k++) {
            sum += A[row * N + k] * B[k * N + j];
        }
        C[row * N + j] = sum;
    }
})";

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

    if (rank == 0) {
        std::cout << "[MPI] Running with " << size << " process(es)\n";
    }

    int rows_per_proc = N / size;

    if (rank == 0) {
        srand(time(NULL));
        fill_matrix();
    }

    MPI_Bcast(B, N * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(A, rows_per_proc * N, MPI_INT, sub_A, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    cl_mem bufA, bufB, bufC;
    cl_int err;

    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);

    bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * rows_per_proc * N, NULL, &err);
    bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * N * N, NULL, &err);
    bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * rows_per_proc * N, NULL, &err);

    err = clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, sizeof(int) * rows_per_proc * N, sub_A, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, sizeof(int) * N * N, B, 0, NULL, NULL);

    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "mat_mul", &err);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &N);
    err |= clSetKernelArg(kernel, 4, sizeof(int), &rows_per_proc);

    size_t global[1] = { static_cast<size_t>(rows_per_proc) };

    double start = MPI_Wtime();
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, NULL);
    clFinish(queue);
    double end = MPI_Wtime();

    err = clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, sizeof(int) * rows_per_proc * N, sub_C, 0, NULL, NULL);

    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    MPI_Gather(sub_C, rows_per_proc * N, MPI_INT, C, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "MPI + OpenCL ececution Time: " << (end - start) << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}

// To compile: mpic++ matrix_mpi_opencl.cpp -lOpenCL -o matrix_mpi_opencl
// To run: mpirun -np 4 ./matrix_mpi_opencl
