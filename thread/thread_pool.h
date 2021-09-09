#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef struct Task {
    void (*function)(void* arg);
    void* arg;
} Task;

typedef struct ThreadPool {
    Task* task_queue;
    int queue_size;
    int queue_cap;
    int front;
    int rear;

    pthread_t manager_id;
    pthread_t* worker_ids;
    int min_num;
    int max_num;
    int busy_num;   // 忙碌线程数
    int live_num;   // 存活线程数
    int exit_num;   // 要销毁的线程个数

    pthread_mutex_t mutex_pool;
    pthread_mutex_t mutex_busy;

    pthread_cond_t empty;
    pthread_cond_t full;

    int shut_down; // 是否要销毁线程池
} ThreadPool;


void* work(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (1) {
        pthread_mutex_lock(&pool->mutex_pool);
        // 判断任务队列是否为空
        while (pool->queue_size == 0 && !pool->shut_down) {
            pthread_cond_wait(&pool->empty, &pool->mutex_pool);
            // 是否需要自杀
            if (pool->exit_num > 0) {
                pool->exit_num--;
                pool->live_num--;
                for (int i = 0; i < pool->max_num; i++) {
                    if (pool->worker_ids[i] == pthread_self()) {
                        pool->worker_ids[i] = 0;
                        break;
                    }
                }
                printf("自杀了一个线程，当前有%d个线程\n", pool->live_num);
                pthread_mutex_unlock(&pool->mutex_pool);
                pthread_exit(NULL);
            }
        }

        // 判断线程池是否被关闭
        if (pool->shut_down) {
            pthread_mutex_unlock(&pool->mutex_pool);
            pthread_exit(NULL);
        }
        
        // 从任务队列中取出一个任务
        Task task;
        task.function = pool->task_queue[pool->front].function;
        task.arg = pool->task_queue[pool->front].arg;
        pool->front = (pool->front+1)%pool->queue_cap;
        pool->queue_size--;
        pthread_cond_signal(&pool->full);
        pthread_mutex_unlock(&pool->mutex_pool);

        printf("tid=%ld start work\n", pthread_self());
        pthread_mutex_lock(&pool->mutex_busy);
        pool->busy_num++;
        pthread_mutex_unlock(&pool->mutex_busy);
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("tid=%ld end work\n", pthread_self());
        pthread_mutex_lock(&pool->mutex_busy);
        pool->busy_num--;
        pthread_mutex_unlock(&pool->mutex_busy);
    }
}

void* manage(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (!pool->shut_down) {
        sleep(5);

        // 添加线程
        // 存活线程个数 < 任务个数 && 存活线程个数 < 最大线程数
        // 添加到min(任务个数,最大个数)个线程
        pthread_mutex_lock(&pool->mutex_pool);
        if (pool->live_num < pool->queue_size && pool->live_num < pool->max_num) {
            int new_num = pool->queue_size > pool->max_num ? pool->max_num : pool->queue_size;
            for (int i = 0; pool->live_num < new_num; i++) {
                if (pool->worker_ids[i] == 0) {
                    pthread_create(&pool->worker_ids[i], NULL, work, pool);
                    pool->live_num++;
                    printf("增加了一个线程，当前有%d个线程\n", pool->live_num);
                }
            }

        }
        pthread_mutex_unlock(&pool->mutex_pool);


        // 销毁一个线程
        // 忙碌线程数*2 < 存活线程数 && 存活线程个数 > 最小线程数
        if (pool->busy_num*2 < pool->live_num && pool->live_num > pool->min_num) {
            pthread_mutex_lock(&pool->mutex_pool);
            pool->exit_num = 1;
            pthread_mutex_unlock(&pool->mutex_pool);
            pthread_cond_signal(&pool->empty);
        }

    }
}

// 创建线程池
ThreadPool* new_thread_pool(int min, int max, int queue_cap) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        puts("malloc ThreadPool fail...");
        return NULL;
    }

    pool->worker_ids = (pthread_t*)malloc(sizeof(pthread_t)*max);
    if (pool->worker_ids == NULL) {
        puts("malloc worker_ids fail...");
        free(pool);
        return NULL;
    }

    memset(pool->worker_ids, 0, sizeof(pthread_t)*max);

    pool->min_num = min;
    pool->max_num = max;
    pool->busy_num = 0;
    pool->live_num = min;
    pool->exit_num = 0;

    if (pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
        pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
        pthread_cond_init(&pool->empty, NULL) != 0 ||
        pthread_cond_init(&pool->full, NULL) != 0) {
        puts("init mutex or cond fail..."); 
        free(pool->worker_ids);
        free(pool);
        return NULL;
    }

    pool->task_queue = (Task*)malloc(sizeof(Task)*queue_cap);
    if (pool->task_queue == NULL) {
        puts("malloc task_queue fail..."); 
        free(pool->worker_ids);
        free(pool);
        return NULL;
    }
    pool->queue_cap = queue_cap;
    pool->queue_size = 0;
    pool->front = 0;
    pool->rear = 0;

    pool->shut_down = 0;

    pthread_create(&pool->manager_id, NULL, manage, pool);
    for (int i = 0; i < min; i++) {
        pthread_create(&pool->worker_ids[i], NULL, work, pool);
    }

    return pool;
}

void destory_thread_pool(ThreadPool* pool) {
    if (pool == NULL) {
        return;
    }
    pool->shut_down = 1;
    pthread_join(pool->manager_id, NULL);
    for (int i = 0; i < pool->live_num; i++) {
        pthread_cond_signal(&pool->empty);
    }
    free(pool->task_queue);
    pool->task_queue = NULL;
    free(pool->worker_ids);
    pool->worker_ids = NULL;
    
    pthread_cond_destroy(&pool->empty);
    pthread_cond_destroy(&pool->full);
    pthread_mutex_destroy(&pool->mutex_busy);
    pthread_mutex_destroy(&pool->mutex_pool);
    
    free(pool);
    pool = NULL;
}

void add_task(ThreadPool* pool, void(*func)(void*), void* arg) {
    pthread_mutex_lock(&pool->mutex_pool);
    while (pool->queue_size == pool->queue_cap && !pool->shut_down) {
        pthread_cond_wait(&pool->full, &pool->mutex_pool);
    }
    if (pool->shut_down) {
        pthread_mutex_unlock(&pool->mutex_pool);
        return;
    }
    pool->task_queue[pool->rear].function = func;
    pool->task_queue[pool->rear].arg = arg;
    pool->rear = ((pool->rear)+1) % pool->queue_cap;
    pool->queue_size++;
    pthread_cond_signal(&pool->empty);
    pthread_mutex_unlock(&pool->mutex_pool);
}

int get_busy_num(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutex_busy);
    int n = pool->busy_num;
    pthread_mutex_unlock(&pool->mutex_busy);
    return n;
}

int get_live_num(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutex_pool);
    int n = pool->live_num;
    pthread_mutex_unlock(&pool->mutex_pool);
    return n;
}