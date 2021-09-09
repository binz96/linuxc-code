#include "thread_pool.h"

void task_fun(void* arg) {
    printf("执行任务：打印：%d\n", *(int*)arg);
    sleep(2);
}

int main() {
    ThreadPool* pool = new_thread_pool(4, 8, 100);

    for (int i = 0; i < 50; i++) {
        int* pint = (int*)malloc(sizeof(int));
        *pint = i;
        add_task(pool, task_fun, pint);
    }

    sleep(30);

    destory_thread_pool(pool);
}