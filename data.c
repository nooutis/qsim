//
// Created by Charlie Carretta on 02/05/2026.
//

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "data.h"
#include "memory.h"
#include "thread.h"

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

double complex_square_mod(Complex a) {
  return (a.real * a.real) + (a.imag * a.imag);
}

void matrix_prod(const Complex *a, const Complex *b, Complex *c, size_t dim) {
  for (size_t i = 0; i < dim; i++) {
    for (size_t j = 0; j < dim; j++) {
      Complex sum = {0, 0};
      for (size_t k = 0; k < dim; k++) {
        Complex prod =
            complex_prod(a[GET_INDEX(i, k, dim)], b[GET_INDEX(k, j, dim)]);
        sum = complex_add(sum, prod);
      }
      c[GET_INDEX(i, j, dim)] = sum;
    }
  }
}

void matrix_vector_prod(const Complex *m, const Complex *v_in, Complex *v_out,
                        size_t dim) {
  for (size_t i = 0; i < dim; i++) {
    Complex sum = {0, 0};
    for (size_t j = 0; j < dim; j++) {
      Complex prod = complex_prod(m[GET_INDEX(i, j, dim)], v_in[j]);
      sum = complex_add(sum, prod);
    }
    v_out[i] = sum;
  }
}

void execute_matrix_matrix_prod(void *arg) {
  MatrixMatrixTask *task = (MatrixMatrixTask *)arg;
  matrix_prod(task->a, task->b, task->c, task->dim);
}

MatrixMatrixTask create_matrix_matrix_task(Complex *a, Complex *b, Complex *c,
                                           size_t dim) {
  MatrixMatrixTask task = {a, b, c, dim};
  return task;
}

void execute_matrix_vector_prod(void *arg) {
  MatrixVectorTask *task = (MatrixVectorTask *)arg;
  matrix_vector_prod(task->m, task->v_in, task->v_out, task->dim);
}

MatrixVectorTask create_matrix_vector_task(Complex *m, Complex *v_in, Complex *v_out, size_t dim) {
  MatrixVectorTask task = {m, v_in, v_out, dim};
  return task;
}

void execute_complex_mod(void *arg) {
  ComplexModTask *task = (ComplexModTask *)arg;
  *(task->res) = complex_square_mod(*(task->a));
}

ComplexModTask create_complex_mod_task(const Complex *a, double *res) {
  ComplexModTask task = {a, res};
  return task;
}

void execute_sample_task(void *arg) {
  SampleTask *task = (SampleTask *)arg;
  size_t sample = gsl_ran_discrete(task->r, task->g);
  if (task->mutex)
    pthread_mutex_lock(task->mutex);
  task->counts[sample]++;
  if (task->mutex)
    pthread_mutex_unlock(task->mutex);
}

SampleTask create_sample_task(gsl_rng *r, gsl_ran_discrete_t *g, size_t *counts, pthread_mutex_t *mutex) {
  SampleTask task = {r, g, counts, mutex};
  return task;
}

int generate_multiplication_task(QuantumCircuit circ, MultiplicationTask *task) {
  task->matrix = malloc(sizeof(ComplexMatrix) * circ.num_gates);
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
  if (dividend % 2 == 0)
    return dividend / 2;
  return dividend / 2 + 1;
}

int calculate_circuit(QuantumCircuit circ, Complex *v_out, size_t num_threads, ThreadPool *tm) {
  size_t dim = DIM(circ.qubits);
  size_t mat_size = dim * dim;

  if (v_out == NULL) {
    fprintf(stderr, "Vettore di output non valido \n");
    return 1;
  }

  if (num_threads == 1) {
    Complex *res = malloc(sizeof(Complex) * mat_size);
    Complex *temp = malloc(sizeof(Complex) * mat_size);

    if (res == NULL || temp == NULL) {
      fprintf(stderr, "Errore nell'allocazione delle matrici \n");
      return 1;
    }

    if (circ.num_gates > 0) {
      memcpy(res, circ.gates[0].matrix, sizeof(Complex) * mat_size);

      for (size_t i = 1; i < circ.num_gates; i++) {
        matrix_prod(circ.gates[i].matrix, res, temp, dim);
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

    matrix_vector_prod(res, circ.state_vector, v_out, dim);
    free(res);
    free(temp);
    return 0;
  }

  if (circ.num_gates == 0) {
    for (size_t i = 0; i < dim; i++) {
      v_out[i] = circ.state_vector[i];
    }
    return 0;
  }

  MultiplicationTask task = {NULL, 0};
  generate_multiplication_task(circ, &task);

  int is_first_task = 1;

  while (task.num_tasks > 1) {
    size_t num_pairs = task.num_tasks / 2;
    size_t remaining = task.num_tasks % 2;
    size_t next_num_tasks = num_pairs + remaining;

    MultiplicationTask next_task = allocate_matrix_tasks(next_num_tasks, mat_size);
    MatrixMatrixTask *args = malloc(sizeof(MatrixMatrixTask) * num_pairs);

    for (size_t i = 0; i < num_pairs; i++) {
      args[i] = create_matrix_matrix_task(task.matrix[2 * i + 1].matrix, task.matrix[2 * i].matrix,next_task.matrix[i].matrix, dim);
      tpool_add_work(tm, execute_matrix_matrix_prod, &args[i]);
    }

    if (remaining) {
      memcpy(next_task.matrix[num_pairs].matrix,
             task.matrix[task.num_tasks - 1].matrix,
             sizeof(Complex) * mat_size);
    }

    tpool_wait(tm);
    free(args);

    if (is_first_task) {
      free(task.matrix);
      is_first_task = 0;
    } else {
      free_matrix_tasks(&task);
    }

    task = next_task;
  }

  MatrixVectorTask *arg_vec = malloc(sizeof(MatrixVectorTask));
  *arg_vec = create_matrix_vector_task(task.matrix[0].matrix, circ.state_vector,
                                       v_out, dim);
  tpool_add_work(tm, execute_matrix_vector_prod, arg_vec);
  tpool_wait(tm);
  free(arg_vec);

  if (is_first_task) {
    free(task.matrix);
  } else {
    free_matrix_tasks(&task);
  }

  return 0;
}

void print_vector(const Complex *vector, size_t size) {
  if (vector == NULL || size <= 0) {
    fprintf(stderr, "Puntatore o dimensione non validi");
    return;
  }

  printf("[");
  // Iterate through each element of the vector
  for (size_t i = 0; i < size; i++) {
    // Calculate the index in the 1D array
    size_t index = i;
    if (i != 0) {
      if (vector[index].imag == 0)
        printf(", %.6g", vector[index].real);
      else
        printf(", %.6g + i%.6g", vector[index].real, vector[index].imag);
    }
    else {
      if (vector[index].imag == 0)
        printf(" %.6g", vector[index].real);
      else
        printf(" %.6g + i%.6g", vector[index].real, vector[index].imag);
    }
  }
  printf(" ]\n");
}
