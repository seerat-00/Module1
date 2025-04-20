#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
using namespace std;
using namespace chrono;

const int N = 1024;

string loadKernel(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open kernel file!" << endl;
        exit(1);
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

int main() {
    vector<int> data(N);
    for (int i = 0; i < N; i++)
        data[i] = rand() % 10000;

    auto start = high_resolution_clock::now();

    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);

    cl_device_id device;
    cl_int err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    
    if (err != CL_SUCCESS) {
        cout << "Error getting GPU device, Trying CPU instead..." << endl;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        if (err != CL_SUCCESS) {
            cerr << "Error getting CPU device!" << endl;
            exit(1);
        }
    }
  
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, nullptr, nullptr);

    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                   sizeof(int) * N, data.data(), nullptr);

    string kernelSource = loadKernel("opencl_sorting.cl");
    const char* source = kernelSource.c_str();
    size_t sourceSize = kernelSource.size();

    cl_program program = clCreateProgramWithSource(context, 1, &source, &sourceSize, nullptr);
    clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);

    cl_kernel kernel = clCreateKernel(program, "quick_sort", nullptr);

    int chunk_size = 64;
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    clSetKernelArg(kernel, 1, sizeof(int), &chunk_size);

    size_t global_size = N / chunk_size;
    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);

    clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, sizeof(int) * N, data.data(), 0, nullptr, nullptr);

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "First 10 sorted values: ";
    for (int i = 0; i < 10; i++) cout << data[i] << " ";
    cout << "\nTime Taken: " << duration.count() << " ms" << endl;

    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
