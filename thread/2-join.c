#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct Retval {
    int age;
    int num;
};

struct Retval retval;   
// 不能放在子线程栈内存
// 可以是全局变量、堆内存、主线程栈空间作为参数传给子线程

void* callback(void * arg){
    printf("tid: %ld\n", pthread_self());
    retval.age = 25;
    retval.num = 100;
    pthread_exit(&retval);
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid,NULL,callback,NULL);
    void* pret = NULL;
    pthread_join(tid,&pret);    // 阻塞
    printf("age: %d\nnum: %d\n",((struct Retval*)pret)->age,((struct Retval*)pret)->num);
    return 0;
}