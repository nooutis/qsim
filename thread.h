// File thread.h, contiene le strutture dati necessarie alla gestione del
// ThreadPool

#ifndef SO2_THREAD_H
#define SO2_THREAD_H

#include <pthread.h>
#include <stdbool.h>

typedef void (*thread_func_t)(void *arg);

typedef struct ThreadPoolWork {
  thread_func_t func;
  void *arg;
  struct ThreadPoolWork *next;
} ThreadPoolWork;

typedef struct ThreadPool {
  ThreadPoolWork *work_first;
  ThreadPoolWork *work_last;
  pthread_mutex_t work_mutex;
  pthread_cond_t work_cond;
  pthread_cond_t working_cond;
  size_t working_cnt;
  size_t num_threads;
  volatile bool stop;
} ThreadPool;

ThreadPool *tpool_create(size_t num);
void tpool_destroy(ThreadPool *tm);
bool tpool_add_work(ThreadPool *tm, thread_func_t func, void *arg);
void tpool_wait(ThreadPool *tm);

#endif // SO2_THREAD_H
