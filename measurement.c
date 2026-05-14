//
// Created by Charlie Carretta on 29/04/2026.
//

#include <math.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "measurement.h"

/**
 * Simula la fase di misurazione di un circuito quantistico.
 * @param v_out       Vettore finale di numeri complessi
 * @param num_threads Numero di thread da usare (
 * @param repetitions Numero di campionamenti da effettuare (es. 100)
 * @param dim         Dimensione del vettore (2^n)
 * @param tm          Puntatore alla struttura ThreadPool
 */

int measure_circuit(const Complex *v_out, size_t num_threads, size_t repetitions, size_t dim, ThreadPool *tm) {
    if (repetitions == 0) return 0;

    double *probabilities = malloc(dim * sizeof(double));
    if (!probabilities) {
        fprintf(stderr, "Errore nell'allocazione del vettore probabilità\n");
        return 1;
    }

    if (num_threads > 1 && tm != NULL) {
        ComplexModTask *tasks = malloc(dim * sizeof(ComplexModTask));
        if (!tasks) {
            free(probabilities);
            return 1;
        }

        for (size_t j = 0; j < dim; j++) {
            tasks[j] = create_complex_mod_task(&v_out[j], &probabilities[j]);
            tpool_add_work(tm, execute_complex_mod, &tasks[j]);
        }
        tpool_wait(tm);
        free(tasks);
    } else {
        for (size_t j = 0; j < dim; j++) {
            probabilities[j] = complex_square_mod(v_out[j]);
        }
    }

    const gsl_rng_type *T = gsl_rng_default;
    gsl_rng *r = gsl_rng_alloc(T);
    gsl_ran_discrete_t *g = gsl_ran_discrete_preproc(dim, probabilities);

    size_t *counts = calloc(dim, sizeof(size_t));
    if (!counts) {
        gsl_ran_discrete_free(g);
        gsl_rng_free(r);
        free(probabilities);
        return 1;
    }

    if (num_threads > 1 && tm != NULL) {
        pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
        SampleTask *s_tasks = malloc(repetitions * sizeof(SampleTask));
        if (s_tasks) {
            for (size_t i = 0; i < repetitions; i++) {
                s_tasks[i] = create_sample_task(r, g, counts, &count_mutex);
                tpool_add_work(tm, execute_sample_task, &s_tasks[i]);
            }
            tpool_wait(tm);
            free(s_tasks);
        } else {
            fprintf(stderr, "Errore nell'allocazione dei task \n");
        }
        pthread_mutex_destroy(&count_mutex);
    } else {
        for (size_t i = 0; i < repetitions; i++) {
            size_t sample = gsl_ran_discrete(r, g);
            counts[sample]++;
        }
    }

    size_t n = (size_t)round(log2((double) dim));
    for (size_t j = 0; j < dim; j++) {
        double estimated_prob = (double)counts[j] / (int) repetitions;
        for (int bit = (int) n - 1; bit >= 0; bit--) {
            printf("%zu", (j >> bit) & 1);
        }
        printf(" @ %.6g\n", estimated_prob);
    }

    gsl_ran_discrete_free(g);
    gsl_rng_free(r);
    free(counts);
    free(probabilities);

    return 0;
}
