//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_THREAD_H
#define SO2_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#include "data.h"

typedef void (*thread_func_t)(void *arg);

typedef struct ThreadPoolWork {
    thread_func_t      func;
    void              *arg;
    struct ThreadPoolWork *next;
} ThreadPoolWork;

typedef struct ThreadPool {
    ThreadPoolWork   *work_first;
    ThreadPoolWork   *work_last;
    pthread_mutex_t  work_mutex;
    pthread_cond_t   work_cond;
    pthread_cond_t   working_cond;
    size_t           working_cnt;
    size_t           thread_cnt;
    bool             stop;
} ThreadPool;

ThreadPool *tpool_create(size_t num);
void tpool_destroy(ThreadPool *tm);
bool tpool_add_work(ThreadPool *tm, thread_func_t func, void *arg);
void tpool_wait(ThreadPool *tm);

int monothread(QuantumCircuit circ, Complex *v_out);
int multithread(QuantumCircuit circ, Complex *v_out, int num_threads);

#endif //SO2_THREAD_H
