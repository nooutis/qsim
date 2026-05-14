//
// Created by Charlie Carretta on 29/04/2026.
//

#include <stdlib.h>

#include "thread.h"
#include "data.h"
#include "measurement.h"

static void *tpool_worker(void *arg);
static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg);
static void tpool_work_destroy(ThreadPoolWork *work);
static ThreadPoolWork *tpool_work_get(ThreadPool *tm);


static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg) {
    if (func == NULL)
        return NULL;

    ThreadPoolWork *work = malloc(sizeof(*work));
    work->func = func;
    work->arg  = arg;
    work->next = NULL;
    return work;
}

static void tpool_work_destroy(ThreadPoolWork *work) {
    if (work == NULL)
        return;
    free(work);
}

static ThreadPoolWork *tpool_work_get(ThreadPool *tm) {

    if (tm == NULL)
        return NULL;

    ThreadPoolWork *work = tm->work_first;

    if (work == NULL)
        return NULL;

    if (work->next == NULL) {
        tm->work_first = NULL;
        tm->work_last  = NULL;
    } else {
        tm->work_first = work->next;
    }

    return work;
}

static void *tpool_worker(void *arg)
{
    ThreadPool      *tm = arg;

    while (1) {
        pthread_mutex_lock(&(tm->work_mutex));

        while (tm->work_first == NULL && !tm->stop)
            pthread_cond_wait(&(tm->work_cond), &(tm->work_mutex));

        if (tm->stop)
            break;

        ThreadPoolWork *work = tpool_work_get(tm);
        tm->working_cnt++;
        pthread_mutex_unlock(&(tm->work_mutex));

        if (work != NULL) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }

        pthread_mutex_lock(&(tm->work_mutex));
        tm->working_cnt--;
        if (!tm->stop && tm->working_cnt == 0 && tm->work_first == NULL)
            pthread_cond_signal(&(tm->working_cond));
        pthread_mutex_unlock(&(tm->work_mutex));
    }

    tm->thread_cnt--;
    pthread_cond_signal(&(tm->working_cond));
    pthread_mutex_unlock(&(tm->work_mutex));
    return NULL;
}

ThreadPool *tpool_create(size_t num)
{
    pthread_t  thread;

    if (num == 0)
        num = 2;

    ThreadPool *tm = calloc(1, sizeof(*tm));
    tm->thread_cnt = num;

    pthread_mutex_init(&(tm->work_mutex), NULL);
    pthread_cond_init(&(tm->work_cond), NULL);
    pthread_cond_init(&(tm->working_cond), NULL);

    tm->work_first = NULL;
    tm->work_last  = NULL;

    for (size_t i=0; i<num; i++) {
        pthread_create(&thread, NULL, tpool_worker, tm);
        pthread_detach(thread);
    }

    return tm;
}

void tpool_destroy(ThreadPool *tm)
{

    if (tm == NULL)
        return;

    pthread_mutex_lock(&(tm->work_mutex));
    ThreadPoolWork *work = tm->work_first;
    ThreadPoolWork *work2 = NULL;
    while (work != NULL) {
        work2 = work->next;
        tpool_work_destroy(work);
        work = work2;
    }
    tm->work_first = NULL;
    tm->stop = true;
    pthread_cond_broadcast(&(tm->work_cond));
    pthread_mutex_unlock(&(tm->work_mutex));

    tpool_wait(tm);

    pthread_mutex_destroy(&(tm->work_mutex));
    pthread_cond_destroy(&(tm->work_cond));
    pthread_cond_destroy(&(tm->working_cond));

    free(tm);
}

bool tpool_add_work(ThreadPool *tm, thread_func_t func, void *arg)
{


    if (tm == NULL)
        return false;

    ThreadPoolWork *work = tpool_work_create(func, arg);
    if (work == NULL)
        return false;

    pthread_mutex_lock(&(tm->work_mutex));
    if (tm->work_first == NULL) {
        tm->work_first = work;
        tm->work_last  = tm->work_first;
    } else {
        tm->work_last->next = work;
        tm->work_last       = work;
    }

    pthread_cond_broadcast(&(tm->work_cond));
    pthread_mutex_unlock(&(tm->work_mutex));

    return true;
}

void tpool_wait(ThreadPool *tm)
{
    if (tm == NULL)
        return;

    pthread_mutex_lock(&(tm->work_mutex));
    while (1) {
        if (tm->work_first != NULL || (!tm->stop && tm->working_cnt != 0) || (tm->stop && tm->thread_cnt != 0)) {
            pthread_cond_wait(&(tm->working_cond), &(tm->work_mutex));
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&(tm->work_mutex));
}

int multithread(QuantumCircuit circ, Complex *v_out, size_t num_threads, size_t repetitions) {
    ThreadPool *tm = tpool_create(num_threads);
    if (calculate_circuit(circ, v_out, num_threads, tm) != 0) {
        return 1;
    }
    if (circ.repetitions == 0) {
        print_vector(v_out, DIM(circ.qubits));
        tpool_destroy(tm);
        return 0;
    }
    if (measure_circuit(v_out, num_threads, repetitions, DIM(circ.qubits), tm) != 0) {
        return 2;
    }
    tpool_destroy(tm);
    return 0;
}

/*int main()
{
    ThreadPool *tm;
    int     *vals;
    size_t   i;

    tm   = tpool_create(num_threads);
    vals = calloc(num_items, sizeof(*vals));

    for (i=0; i<num_items; i++) {
        vals[i] = i;
        tpool_add_work(tm, worker, vals+i);
    }

    tpool_wait(tm);

    for (i=0; i<num_items; i++) {
        printf("%d\n", vals[i]);
    }

    free(vals);
    tpool_destroy(tm);
    return 0;
}*/