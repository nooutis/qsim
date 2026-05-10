//
// Created by Charlie Carretta on 02/05/2026.
//

#include <math.h>
#include <stdlib.h>
#include <_stdio.h>

#include "data.h"

Complex complex_add(Complex a, Complex b) {
    Complex c = {0, 0};
    c.real = a.real + b.real;
    c.imag = a.imag + b.imag;
    return c;
}

Complex complex_prod(Complex a, Complex b) {
    Complex c = {0, 0};
    c.real = a.real * b.real - a.imag * b.imag;
    c.imag = a.real * b.imag + a.imag * b.real;
    return c;
}

double complex_mod(Complex a) {
    return sqrt(a.real * a.real + a.imag * a.imag);
}

void matrix_prod(const Complex *a, const Complex *b, Complex *c, size_t dim) {
    for (size_t i = 0; i < dim; i++) {
        for (size_t j = 0; j < dim; j++) {
            Complex sum = {0, 0};
            for (size_t k = 0; k < dim; k++) {
                Complex prod = complex_prod(a[GET_INDEX(i, k, dim)], b[GET_INDEX(k, j, dim)]);
                sum = complex_add(sum, prod);
            }
            c[GET_INDEX(i, j, dim)] = sum;
        }
    }
}

void matrix_vector_prod(const Complex *m, const Complex *v_in, Complex *v_out, size_t dim) {
    for (size_t i = 0; i < dim; i++) {
        Complex sum = {0, 0};
        for (size_t j = 0; j < dim; j++) {
            Complex prod = complex_prod(m[GET_INDEX(i,j,dim)], v_in[j]);
            sum = complex_add(sum, prod);
        }
        v_out[i] = sum;
    }
}

void execute_matrix_matrix_prod(void *arg) {
    MatrixMatrixTask *task = (MatrixMatrixTask *)arg;
    matrix_prod(
        task->a,
        task->b,
        task->c,
        task->dim
    );
}

MatrixMatrixTask create_matrix_matrix_task(Complex *a, Complex *b, Complex *c, size_t dim) {
    MatrixMatrixTask task = {a,b,c,dim};
    return task;
}

void execute_matrix_vector_prod(void *arg) {
    MatrixVectorTask *task = (MatrixVectorTask *)arg;
    matrix_vector_prod(
        task->m,
        task->v_in,
        task->v_out,
        task->dim
    );
}

MatrixVectorTask create_matrix_vector_task(Complex *m, Complex *v_in, Complex *v_out, size_t dim) {
    MatrixVectorTask task = {m,v_in,v_out,dim};
    return task;
}

int generate_multiplication_task(QuantumCircuit circ, MultiplicationTask *task) {
    task->matrix = malloc(sizeof(ComplexMatrix)*circ.num_gates);
    if (task->matrix == NULL) {
        fprintf(stderr, "Fallita l'allocazione di task");
        return 1;
    }
    for (size_t i = 0; i < circ.num_gates; i++) {
        task->matrix[i].matrix = circ.gates[i].matrix;
        task->num_tasks++;
    }
    return 0;
}

size_t divide2(size_t dividend) {
    if (dividend % 2 == 0) return dividend / 2;
    return dividend / 2 + 1;
}



