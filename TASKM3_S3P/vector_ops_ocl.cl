// OpenCL kernel function for elemen vector addition
__kernel void vector_add_ocl(const int size, 
                             __global int *v1, 
                             __global int *v2, 
                             __global int *v_out) {

    // Get the global index of the current work item
    const int globalIndex = get_global_id(0);

    // Ensure the index is within the bounds of the array
    if (globalIndex < size) {

        // Perform the addition of the two input vectors
        v_out[globalIndex] = v1[globalIndex] + v2[globalIndex];
    }
}
