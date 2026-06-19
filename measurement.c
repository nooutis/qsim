// File measurement.c, contiene la funzione necessaria alla misurazione del circuito


#include <math.h>
#include <time.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "measurement.h"

/**
 * Esegue la misurazione del circuito quantistico.
 * 
 * @param v_out Vettore di stato finale (numeri complessi).
 * @param num_threads Numero di thread da utilizzare.
 * @param repetitions Numero di campionamenti da eseguire.
 * @param dim Dimensione del vettore di stato (2^n).
 * @param tm Puntatore al thread pool.
 * @return 0 in caso di successo, 1 in caso di errore di allocazione.
 */
int measure_circuit(const Complex *v_out, size_t num_threads, size_t repetitions, size_t dim, ThreadPool *tm) {
    if (repetitions == 0) return 0;

    // Allocazione del vettore delle probabilità (|alpha_j|^2)
    double *probabilities = malloc(dim * sizeof(double));
    if (!probabilities) {
        fprintf(stderr, "Errore nell'allocazione del vettore probabilità\n");
        return 1;
    }

    // Caso di esecuzione multi-threaded
    if (num_threads > 1 && tm != NULL) {
        // Calcolo dei moduli quadri in parallelo
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
    }
    // Caso di esecuzione single-threaded
    else {
        for (size_t j = 0; j < dim; j++) {
            probabilities[j] = complex_square_mod(v_out[j]);
        }
    }

    // Campionamento usando la GSL
    const gsl_rng_type *T = gsl_rng_default;
    gsl_rng *r = gsl_rng_alloc(T);
    gsl_rng_set(r, (unsigned long int)time(NULL)); // Imposta come seme il tempo corrente
    gsl_ran_discrete_t *g = gsl_ran_discrete_preproc(dim, probabilities);

    size_t *counts = calloc(dim, sizeof(size_t));
    if (!counts) {
        gsl_ran_discrete_free(g);
        gsl_rng_free(r);
        free(probabilities);
        return 1;
    }

    if (num_threads > 1 && tm != NULL) {
        // Campionamento parallelo (protetto da mutex)
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
            // Fallback in caso di errore di allocazione
            for (size_t i = 0; i < repetitions; i++) {
                size_t sample = gsl_ran_discrete(r, g);
                counts[sample]++;
            }
        }
        pthread_mutex_destroy(&count_mutex);
    } else {
        // Campionamento sequenziale
        for (size_t i = 0; i < repetitions; i++) {
            size_t sample = gsl_ran_discrete(r, g);
            counts[sample]++;
        }
    }

    // Stampa la distribuzione stimata nel formato "binario @ probabilità"
    size_t n = (size_t)round(log2((double) dim));
    for (size_t j = 0; j < dim; j++) {
        double estimated_prob = (double)counts[j] / (int) repetitions;
        for (int bit = (int) n - 1; bit >= 0; bit--) {
            printf("%zu", (j >> bit) & 1);
        }
        printf(" @ %.6g\n", estimated_prob);
    }

    // Pulizia
    gsl_ran_discrete_free(g);
    gsl_rng_free(r);
    free(counts);
    free(probabilities);

    return 0;
}
