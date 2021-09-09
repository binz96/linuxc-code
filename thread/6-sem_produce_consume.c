#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

pthread_mutex_t mutex;
sem_t semp; // 可生产的资源数
sem_t semc; // 可消费的资源数


struct queue {
    int data[10];
    int size;
} queue;

void* produce(void* arg) {
    while (1) {
        sem_wait(&semp);
        pthread_mutex_lock(&mutex);
        int newn = rand() % 10;
        queue.data[queue.size] = newn;
        queue.size++;
        printf("add num: %d\n", newn);
        for (int i = 0; i < queue.size; i++) {
            printf("%d ", queue.data[i]);
        }
        puts("");
        sem_post(&semc);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
}

void* consume(void* arg){
    while (1) {
        sem_wait(&semc);
        pthread_mutex_lock(&mutex);
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

        sem_post(&semp);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semp, 0, 10);
    sem_init(&semc, 0, 0);

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
    sem_destroy(&semp);
    sem_destroy(&semc);
}