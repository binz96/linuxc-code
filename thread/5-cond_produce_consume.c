#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t empty;
pthread_cond_t full;

struct queue {
    int data[10];
    int size;
} queue;

void* produce(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (queue.size == 10) {
            pthread_cond_wait(&full, &mutex);
        }
        int newn = rand() % 10;
        queue.data[queue.size] = newn;
        queue.size++;
        printf("add num: %d\n", newn);
        for (int i = 0; i < queue.size; i++) {
            printf("%d ", queue.data[i]);
        }
        puts("");
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&empty);
        sleep(1);
    }
}

void* consume(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (queue.size == 0) {
            pthread_cond_wait(&empty, &mutex);
        }
        int first = queue.data[0];
        for (int i = 1; i < queue.size; i++) {
            queue.data[i-1] = queue.data[i];
        }
        queue.size--;
        printf("pop num: %d\n", first);
        for (int i = 0; i < queue.size; i++) {
            printf("%d ", queue.data[i]);
        }
        puts("");
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&full);
        sleep(1);
    }
}

int main() {
    // struct timespec tspc;
    // tspc.tv_nsec = 0;
    // tspc.tv_sec = time(NULL)+100;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&full, NULL);

    pthread_t producer[5], consumer[5];

    for (int i = 0; i < 5; i++) {
        pthread_create(&producer[i], NULL, produce, NULL);
    }
        pthread_create(&consumer[0], NULL, consume, NULL);

    for (int i = 0; i < 5; i++) {
        pthread_join(producer[i], NULL);
    }
        pthread_join(consumer[0], NULL);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&empty);
    pthread_cond_destroy(&full);
}