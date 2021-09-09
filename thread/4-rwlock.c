#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int total = 0;
pthread_rwlock_t rwlock;

void* read(void* arg) {
    for (int i = 0; i < 500000; i++) {
        pthread_rwlock_rdlock(&rwlock);
        printf("tid: %ld: read total = %d\n", pthread_self(),total);
        pthread_rwlock_unlock(&rwlock);
    }
    return NULL;
}

void* write(void* arg) {
    for (int i = 0; i < 500000; i++) {
        pthread_rwlock_wrlock(&rwlock);
        total++;
        printf("tid: %ld: write total = %d\n", pthread_self(),total);
        pthread_rwlock_unlock(&rwlock);
    }
    return NULL;
}

int main() {
    pthread_rwlock_init(&rwlock, NULL);
    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, read, NULL);
    pthread_create(&t2, NULL, write, NULL);
    pthread_create(&t3, NULL, write, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_rwlock_destroy(&rwlock);
    printf("total = %d\n", total);
}