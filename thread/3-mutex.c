#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int total = 0;
pthread_mutex_t mutex;

void* callback(void * arg){
    for (int i = 0; i < 500000; i++) {
        pthread_mutex_lock(&mutex);
        total++;
        printf("tid: %ld: total = %d\n", pthread_self(),total);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, callback, NULL);
    pthread_create(&t2, NULL, callback, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&mutex);
    printf("total = %d\n", total);
}