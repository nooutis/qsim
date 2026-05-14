//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_DATA_H
#define SO2_DATA_H

#include <gsl/gsl_randist.h>

#include "thread.h"

#define GET_INDEX(row, col, dim) (((row) * (dim)) + (col))

#define DIM(n) ((size_t)1 << n)

typedef struct Complex {
    double real;
    double imag;
} Complex;

typedef struct QuantumGate {
    char name[32];
    Complex *matrix;
    size_t dim;
} QuantumGate;

typedef struct GateRegistry {
    QuantumGate *gates;
    size_t count;
    size_t capacity;
} GateRegistry;

typedef struct QuantumCircuit {
    size_t qubits;
    Complex *state_vector;
    QuantumGate *gates;
    size_t num_gates;
    size_t capacity;
    size_t repetitions;
} QuantumCircuit;

typedef struct ComplexMatrix {
    Complex *matrix;
} ComplexMatrix;

typedef struct MultiplicationTask {
    ComplexMatrix *matrix;
    size_t num_tasks;
} MultiplicationTask;

typedef struct MatrixVectorTask {
    const Complex *m;
    const Complex *v_in;
    Complex *v_out;
    size_t dim;
} MatrixVectorTask;

typedef struct MatrixMatrixTask {
    const Complex *a;
    const Complex *b;
    Complex *c;
    size_t dim;
} MatrixMatrixTask;

typedef struct ComplexModTask {
    const Complex *a;
    double *res;
} ComplexModTask;

typedef struct SampleTask {
    gsl_rng *r;
    gsl_ran_discrete_t *g;
    size_t *counts;
    pthread_mutex_t *mutex;
} SampleTask;

Complex complex_add(Complex a, Complex b);
Complex complex_prod(Complex a, Complex b);
double complex_square_mod(Complex a);

void matrix_prod(const Complex *a, const Complex *b, Complex *c, size_t dim);
void matrix_vector_prod(const Complex *m, const Complex *v_in, Complex *v_out, size_t dim);

void execute_matrix_matrix_prod(void *arg);
MatrixMatrixTask create_matrix_matrix_task(Complex *a, Complex *b, Complex *c, size_t dim);
void execute_matrix_vector_prod(void *arg);
MatrixVectorTask create_matrix_vector_task(Complex *m, Complex *v_in, Complex *v_out, size_t dim);
void execute_complex_mod(void *arg);
ComplexModTask create_complex_mod_task(const Complex *a, double *res);
void execute_sample_task(void *arg);
SampleTask create_sample_task(gsl_rng *r, gsl_ran_discrete_t *g, size_t *counts, pthread_mutex_t *mutex);

int generate_multiplication_task(QuantumCircuit circ, MultiplicationTask *task);
size_t divide2(size_t dividend);

int calculate_circuit(QuantumCircuit circ, Complex *v_out, size_t num_threads, ThreadPool *tm);
int multithread(QuantumCircuit circ, Complex *v_out, size_t num_threads, size_t repetitions);

void print_vector(const Complex *vector, size_t size);

#endif //SO2_DATA_H