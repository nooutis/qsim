/* File data.c, contiene le funzioni necessarie al simulatore
 * sia per la gestione dell'input che per la gestione del multithreading
 * e della misurazione del circuito
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <tgmath.h>

#include "data.h"
#include "memory.h"
#include "thread.h"

/**
 * Somma due numeri complessi.
 * @param a Primo addendo.
 * @param b Secondo addendo.
 * @return La somma (a + b).
 */
Complex complex_add(Complex a, Complex b) {
  Complex c = {0, 0};
  c.real = a.real + b.real;
  c.imag = a.imag + b.imag;
  return c;
}

/**
 * Moltiplica due numeri complessi.
 * @param a Primo fattore.
 * @param b Secondo fattore.
 * @return Il prodotto (a * b).
 */
Complex complex_prod(Complex a, Complex b) {
  Complex c = {0, 0};
  c.real = a.real * b.real - a.imag * b.imag;
  c.imag = a.real * b.imag + a.imag * b.real;
  return c;
}

/**
 * Calcola il modulo quadro di un numero complesso (|z|^2).
 * @param a Il numero complesso.
 * @return Il modulo quadro (a.real^2 + a.imag^2).
 */
double complex_square_mod(Complex a) {
  return (a.real * a.real) + (a.imag * a.imag);
}

/**
 * Esegue il prodotto righe per colonne tra due matrici quadrate.
 * @param a Prima matrice.
 * @param b Seconda matrice.
 * @param c Matrice risultato.
 * @param dim Lunghezza della riga/colonna della matrice quadrata.
 */
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

/**
 * Esegue il prodotto matrice-vettore.
 * @param m Matrice quadrata.
 * @param v_in Vettore di input.
 * @param v_out Vettore di output.
 * @param dim Dimensione della matrice e del vettore.
 */
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

/**
 * Wrapper per l'esecuzione del prodotto matrice-matrice via ThreadPool.
 * @param arg Contiene gli argomenti necessari per l'esecuzione del prodotto
 * matriciale, in una struttura MatrixMatrixTask
 */
void execute_matrix_matrix_prod(void *arg) {
  MatrixMatrixTask *task = (MatrixMatrixTask *)arg;
  matrix_prod(task->a, task->b, task->c, task->dim);
}

/**
 * Crea una struttura task per il prodotto matrice-matrice.
 * @param a Prima matrice
 * @param b Seconda matrice
 * @param c Matrice risultato
 * @param dim Dimensione delle matrici (dim x dim).
 * @return Una struttura wrapper per l'esecuzione del prodotto matriciale
 */
MatrixMatrixTask create_matrix_matrix_task(Complex *a, Complex *b, Complex *c,
                                           size_t dim) {
  MatrixMatrixTask task = {a, b, c, dim};
  return task;
}

/**
 * Wrapper per l'esecuzione del prodotto matrice-vettore via ThreadPool.
 * @param arg Contiene gli argomenti necessari per l'esecuzione del prodotto
 * matriciale, in una struttura MatrixVectorTask
 */
void execute_matrix_vector_prod(void *arg) {
  MatrixVectorTask *task = (MatrixVectorTask *)arg;
  matrix_vector_prod(task->m, task->v_in, task->v_out, task->dim);
}

/**
 * Crea una struttura task per il prodotto matrice-vettore.
 * @param m Matrice quadrata.
 * @param v_in Vettore di input.
 * @param v_out Vettore di output.
 * @param dim Dimensione della matrice e del vettore.
 * @return Una struttura wrapper per l'esecuzione del prodotto matrice-vettore
 */
MatrixVectorTask create_matrix_vector_task(Complex *m, Complex *v_in,
                                           Complex *v_out, size_t dim) {
  MatrixVectorTask task = {m, v_in, v_out, dim};
  return task;
}

/**
 * Wrapper per il calcolo del modulo quadro via ThreadPool.
 * @param arg Contiene gli argomenti necessari per l'esecuzione del modulo
 * quadro, in una struttura ComplexModTask
 */
void execute_complex_mod(void *arg) {
  ComplexModTask *task = (ComplexModTask *)arg;
  *(task->res) = complex_square_mod(*(task->a));
}

/**
 * Crea una struttura task per il calcolo del modulo quadro.
 * @param a Il numero complesso.
 * @param res Il risultato del calcolo del modulo quadro
 * @return Una struttura wrapper per il calcolo del modulo quadro
 */
ComplexModTask create_complex_mod_task(const Complex *a, double *res) {
  ComplexModTask task = {a, res};
  return task;
}

/**
 * Wrapper per l'esecuzione del campionamento via ThreadPool.
 * @param arg Contiene gli argomenti necessari per l'esecuzione del
 * campionamento via ThreadPool, in una struttura SampleTask
 */
void execute_sample_task(void *arg) {
  SampleTask *task = (SampleTask *)arg;
  size_t sample = gsl_ran_discrete(task->r, task->g);
  if (task->mutex)
    pthread_mutex_lock(task->mutex);
  task->counts[sample]++;
  if (task->mutex)
    pthread_mutex_unlock(task->mutex);
}

/**
 * Crea una struttura task per il campionamento.
 * @param r Il generatore di numeri casuali
 * @param g La distribuzione di probabilità discreta
 * @param counts Il vettore che tiene il conto dei valori campionati
 * @param mutex Il puntatore al mutex utilizzato per la sincronizzazione dei
 * thread
 */
SampleTask create_sample_task(gsl_rng *r, gsl_ran_discrete_t *g, size_t *counts,
                              pthread_mutex_t *mutex) {
  SampleTask task = {r, g, counts, mutex};
  return task;
}

/**
 * Genera il task di moltiplicazione iniziale.
 * @param circ Il circuito da cui estrarre il task di moltiplicazione
 * @param task La struttura che conterrà le informazioni del task di
 * moltiplicazione
 */
int generate_multiplication_task(QuantumCircuit circ,
                                 MultiplicationTask *task) {
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

/**
 * La funzione principale che si occupa di calcolare l'evoluzione del circuito
 * quantistico
 * @param circ Il circuito quantistico di cui calcolare l'evoluzione
 * @param v_out Il vettore di output che conterrà lo stato finale del circuito
 * @param num_threads Il numero di thread da utilizzare per la misurazione
 * @param tm Il puntatore al ThreadPool da utilizzare per il multithreading
 * @return 0 se la funzione è riuscita con successo, 1 altrimenti
 */
int calculate_circuit(QuantumCircuit circ, Complex *v_out, size_t num_threads,
                      ThreadPool *tm) {
  size_t dim = DIM(circ.qubits);
  size_t mat_size = dim * dim;

  // Verifica se il vettore di output è valido
  if (v_out == NULL) {
    fprintf(stderr, "Vettore di output non valido \n");
    return 1;
  }

  // Se non ci sono porte logiche nel circuito, il vettore di output è uguale al
  // vettore di stato iniziale
  if (circ.num_gates == 0) {
    for (size_t i = 0; i < dim; i++) {
      v_out[i] = circ.state_vector[i];
    }
    return 0;
  }

  // Esecuzione del programma in modalità single-threaded
  if (num_threads == 1) {
    Complex *res = malloc(sizeof(Complex) * mat_size);
    Complex *temp = malloc(sizeof(Complex) * mat_size);

    // Verifica che le due matrici complesse siano allocate correttamente
    if (res == NULL || temp == NULL) {
      fprintf(stderr, "Errore nell'allocazione delle matrici \n");
      return 1;
    }

    memcpy(res, circ.gates[0].matrix, sizeof(Complex) * mat_size);
    for (size_t i = 1; i < circ.num_gates; i++) {
      matrix_prod(circ.gates[i].matrix, res, temp, dim);
      memcpy(res, temp, sizeof(Complex) * mat_size);
    }
    matrix_vector_prod(res, circ.state_vector, v_out, dim);
    free(res);
    free(temp);
    return 0;
  }

  // Esecuzione del programma in modalità multithreaded
  MultiplicationTask task = {NULL, 0};
  generate_multiplication_task(circ, &task);

  int is_first_task = 1;

  // Il ciclo che permette di iterare le moltiplicazioni finché ci sono
  // operazioni da eseguire
  while (task.num_tasks > 1) {
    size_t num_pairs = task.num_tasks / 2;
    size_t remaining = task.num_tasks % 2;
    size_t next_num_tasks = num_pairs + remaining;

    MultiplicationTask next_task =
        allocate_matrix_tasks(next_num_tasks, mat_size);
    MatrixMatrixTask *args = malloc(sizeof(MatrixMatrixTask) * num_pairs);

    // Inserisce le moltiplicazioni nel ThreadPool
    for (size_t i = 0; i < num_pairs; i++) {
      args[i] = create_matrix_matrix_task(task.matrix[2 * i + 1].matrix,
                                          task.matrix[2 * i].matrix,
                                          next_task.matrix[i].matrix, dim);
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

  // La moltiplicazione finale tra il risultato della moltiplicazione matriciale
  // e il vettore di stato iniziale
  MatrixVectorTask *arg_vec = malloc(sizeof(MatrixVectorTask));
  *arg_vec = create_matrix_vector_task(task.matrix[0].matrix, circ.state_vector,
                                       v_out, dim);
  tpool_add_work(tm, execute_matrix_vector_prod, arg_vec);
  tpool_wait(tm);

  // Pulizia
  free(arg_vec);
  if (is_first_task) {
    free(task.matrix);
  } else {
    free_matrix_tasks(&task);
  }

  return 0;
}

/** Stampa a schermo il vettore
 * @param vector Il vettore da stampare
 * @param size le dimensioni del vettore da stampare
 */
void print_vector(const Complex *vector, size_t size) {

  // Controlla la validità del vettore e della dimensione
  if (vector == NULL || size <= 0) {
    fprintf(stderr, "Puntatore o dimensione non validi");
    return;
  }

  // Stampa effettiva del vettore, distinguendo il segno della parte immaginaria
  printf("[");
  if (vector[0].imag > 0)
    printf(" %.6g + i%.6g", vector[0].real, vector[0].imag);
  else if (vector[0].imag == 0)
    printf(" %.6g", vector[0].real);
  else if (vector[0].imag < 0)
    printf(" %.6g - i%.6g", vector[0].real, fabs(vector[0].imag));

  for (size_t i = 1; i < size; i++) {
    if (vector[i].imag > 0)
      printf(", %.6g + i%.6g", vector[i].real, vector[i].imag);
    else if (vector[i].imag == 0)
      printf(", %.6g", vector[i].real);
    else if (vector[i].imag < 0)
      printf(", %.6g - i%.6g", vector[i].real, fabs(vector[i].imag));
  }
  printf(" ]\n");
}
