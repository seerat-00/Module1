__kernel void quick_sort(__global int* arr, int chunk_size) {
    int gid = get_global_id(0);
    int start = gid * chunk_size;
    int end = start + chunk_size - 1;

    int stack[300000];
    int top = -1;

    if (start >= end) return;

    stack[++top] = start;
    stack[++top] = end;

    while (top >= 0) {
        end = stack[top--];
        start = stack[top--];

        int pivot = arr[end];
        int i = start - 1;

        for (int j = start; j < end; j++) {
            if (arr[j] <= pivot) {
                i++;
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }

        int tmp = arr[i + 1];
        arr[i + 1] = arr[end];
        arr[end] = tmp;

        int pi = i + 1;

        if (pi - 1 > start) {
            stack[++top] = start;
            stack[++top] = pi - 1;
        }

        if (pi + 1 < end) {
            stack[++top] = pi + 1;
            stack[++top] = end;
        }
    }
}
