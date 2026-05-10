//
// Created by Charlie Carretta on 29/04/2026.
//

#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memory.h"

static void *tpool_worker(void *arg);
static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg);
static void tpool_work_destroy(ThreadPoolWork *work);
static ThreadPoolWork *tpool_work_get(ThreadPool *tm);


static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg) {
    ThreadPoolWork *work;

    if (func == NULL)
        return NULL;

    work       = malloc(sizeof(*work));
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
    ThreadPoolWork *work;

    if (tm == NULL)
        return NULL;

    work = tm->work_first;
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
    ThreadPoolWork *work;

    while (1) {
        pthread_mutex_lock(&(tm->work_mutex));

        while (tm->work_first == NULL && !tm->stop)
            pthread_cond_wait(&(tm->work_cond), &(tm->work_mutex));

        if (tm->stop)
            break;

        work = tpool_work_get(tm);
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
    ThreadPool   *tm;
    pthread_t  thread;
    size_t     i;

    if (num == 0)
        num = 2;

    tm             = calloc(1, sizeof(*tm));
    tm->thread_cnt = num;

    pthread_mutex_init(&(tm->work_mutex), NULL);
    pthread_cond_init(&(tm->work_cond), NULL);
    pthread_cond_init(&(tm->working_cond), NULL);

    tm->work_first = NULL;
    tm->work_last  = NULL;

    for (i=0; i<num; i++) {
        pthread_create(&thread, NULL, tpool_worker, tm);
        pthread_detach(thread);
    }

    return tm;
}

void tpool_destroy(ThreadPool *tm)
{
    ThreadPoolWork *work;
    ThreadPoolWork *work2;

    if (tm == NULL)
        return;

    pthread_mutex_lock(&(tm->work_mutex));
    work = tm->work_first;
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
    ThreadPoolWork *work;

    if (tm == NULL)
        return false;

    work = tpool_work_create(func, arg);
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

int monothread(QuantumCircuit circuit, Complex *v_out) {
    size_t dim = DIM(circuit.qubits);
    size_t mat_size = dim * dim;
    Complex *res = malloc(sizeof(Complex) * mat_size);
    Complex *temp = malloc(sizeof(Complex) * mat_size);

    if (res == NULL || temp == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for matrices.\n");
        return 1;
    }

    if (circuit.num_gates > 0) {
        memcpy(res, circuit.gates[0].matrix, sizeof(Complex) * mat_size);

        for (size_t i = 1; i < circuit.num_gates; i++) {
            matrix_prod(circuit.gates[i].matrix, res, temp, dim);
            memcpy(res, temp, sizeof(Complex) * mat_size);
        }
    } else {

        for (size_t i = 0; i < dim; i++) {
            for (size_t j = 0; j < dim; j++) {
                res[GET_INDEX(i, j, dim)] =
                    (i == j) ? (Complex){1.0, 0.0} : (Complex){0.0, 0.0};
            }
        }
    }

    if (v_out == NULL) {
        fprintf(stderr, "Vettore di output non valido \n");
        free(res);
        free(temp);
        return 1;
    }

    matrix_vector_prod(res, circuit.state_vector, v_out, dim);
    free(res);
    free(temp);
    return 0;
}

int multithread(QuantumCircuit circ, Complex *v_out, int num_threads) {
    size_t dim = DIM(circ.qubits);
    size_t mat_size = dim * dim;
    MultiplicationTask task = {NULL, 0};
    generate_multiplication_task(circ, &task);
    size_t num_tasks = task.num_tasks;
    ThreadPool *tm = tpool_create(num_threads);
    for (size_t i = num_tasks; i > 1; i = divide2(i)) {
       MultiplicationTask temp_task = allocate_matrix_tasks(i, mat_size);
        size_t index[2] = {temp_task.num_tasks, temp_task.num_tasks - 1};
        for (size_t j = 1; j < temp_task.num_tasks; j++) {
            MatrixMatrixTask arg = create_matrix_matrix_task(task.matrix[index[0]].matrix, task.matrix[index[1]].matrix, temp_task.matrix[j].matrix, dim);
            tpool_add_work(tm, execute_matrix_matrix_prod,&arg);
            if (index[1] > 1) { index[0]-=2; index[1]-=2; }
            if (index[1] == 1) { temp_task.matrix[0].matrix = task.matrix[0].matrix; }
        }
        reallocate_matrix_tasks(&task, i, mat_size);
        task = temp_task;
        free_matrix_tasks(&temp_task);
    }
    MatrixVectorTask arg = create_matrix_vector_task(task.matrix[0].matrix, circ.state_vector, v_out, dim);
    tpool_add_work(tm, execute_matrix_vector_prod,&arg);
    free_matrix_tasks(&task);
    tpool_destroy(tm);
    return 0;
}

void worker(void *arg)
{
    int *val = arg;
    int  old = *val;

    *val += 1000;
    printf("tid=%p, old=%d, val=%d\n", pthread_self(), old, *val);

    if (*val%2)
        usleep(1000000);
    //printf("Prova");
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