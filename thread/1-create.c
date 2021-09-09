#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* callback(void * arg){
    printf("tid: %ld\n", pthread_self());
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid,NULL,callback,NULL);
    pthread_detach(tid);
    pthread_exit(NULL);
}