#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int THREAD_COUNT = 4;
unsigned long vectorSize = 10000;

int *v1, *v2, *v3;
long long totalSum = 0; // total sum of v3 elements
pthread_mutex_t sumMutex;

struct ThreadData {
    unsigned long start;
    unsigned long end;
};

void* addVectors(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    long long localSum = 0;

    for (unsigned long i = data->start; i < data->end; ++i) {
        v3[i] = v1[i] + v2[i];
        localSum += v3[i];
    }

    // Lock before updating global total sum
    pthread_mutex_lock(&sumMutex);
    totalSum += localSum;
    pthread_mutex_unlock(&sumMutex);

    pthread_exit(NULL);
}

void randomVector(int vector[], unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        vector[i] = rand() % 100;
    }
}

int main() {
    srand(time(0));
    pthread_mutex_init(&sumMutex, NULL);

    v1 = (int*)malloc(vectorSize * sizeof(int));
    v2 = (int*)malloc(vectorSize * sizeof(int));
    v3 = (int*)malloc(vectorSize * sizeof(int));

    randomVector(v1, vectorSize);
    randomVector(v2, vectorSize);

    pthread_t threads[THREAD_COUNT];
    ThreadData thread_data[THREAD_COUNT];
    unsigned long chunk = vectorSize / THREAD_COUNT;

    auto start = high_resolution_clock::now();

    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].start = i * chunk;
        thread_data[i].end = (i == THREAD_COUNT - 1) ? vectorSize : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, addVectors, &thread_data[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Total sum of v3: " << totalSum << endl;
    cout << "Time taken: " << duration.count() << " microseconds" << endl;

    free(v1);
    free(v2);
    free(v3);
    pthread_mutex_destroy(&sumMutex);

    return 0;
}
