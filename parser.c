//
// Created by Charlie Carretta on 29/04/2026.
//

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_main(int argc, const char *argv[], int *thread_number,
               QuantumCircuit *circ) {
  char *end_pointer = NULL;
  char qubits_line[QUBITS_LEN];
  size_t qubits;

  if (argc < 4) {
    fprintf(stderr,
            "Numero di Parametri Insufficiente. Uso: %s <file1> <file2> "
            "<n_thread>\n",
            argv[0]);
    return 1;
  }
  FILE *f1 = fopen(argv[1], "r");
  FILE *f2 = fopen(argv[2], "r");

  if (f1 == NULL || f2 == NULL) {
    fprintf(stderr, "Errore nella lettura dei file\n");
    if (f1)
      fclose(f1);
    if (f2)
      fclose(f2);
    return 2;
  }

  *thread_number = (int)strtol(argv[3], &end_pointer, 10);
  if (*end_pointer != '\0' || *thread_number <= 0) {
    fprintf(stderr, "Il numero di thread deve essere un intero positivo\n");
    fclose(f1);
    fclose(f2);
    return 3;
  }

  if (fgets(qubits_line, QUBITS_LEN, f1) == NULL) {
    fprintf(stderr, "Errore nella lettura dei qubits\n");
    fclose(f1);
    fclose(f2);
    return 4;
  }

  if (parse_qubits(qubits_line, &qubits) != 0) {
    fclose(f1);
    fclose(f2);
    return 5;
  }
  circ->qubits = qubits;
  size_t dim = DIM(qubits);

  char *init_line = malloc(INIT_LEN(qubits));
  if (init_line == NULL) {
    fprintf(stderr, "Errore di allocazione di init\n");
    fclose(f1);
    fclose(f2);
    return 6;
  }

  do {
    if (fgets(init_line, INIT_LEN(qubits), f1) == NULL) {
      fprintf(stderr, "Errore nella lettura del valore iniziale\n");
      free(init_line);
      fclose(f1);
      fclose(f2);
      return 7;
    }
  } while (strcmp(init_line, "\n") == 0 || strcmp(init_line, "\r\n") == 0);

  circ->state_vector = malloc(dim * sizeof(Complex));
  if (circ->state_vector == NULL) {
    fprintf(stderr, "Errore di allocazione per state_vector\n");
    free(init_line);
    fclose(f1);
    fclose(f2);
    return 8;
  }

  if (parse_init(init_line, dim, circ->state_vector) != 0) {
    free(init_line);
    free(circ->state_vector);
    circ->state_vector = NULL;
    fclose(f1);
    fclose(f2);
    return 9;
  }

  free(init_line);
  fclose(f1);

  GateRegistry reg;
  reg.count = 0;
  reg.capacity = 4;
  reg.gates = malloc(reg.capacity * sizeof(QuantumGate));
  if (reg.gates == NULL) {
    fprintf(stderr, "Errore di allocazione del registro dei gate\n");
    fclose(f2);
    return 10;
  }

  char *define_line = malloc(DEFINE_LEN(qubits));
  if (define_line == NULL) {
    fprintf(stderr, "Errore di allocazione per define_line\n");
    free(reg.gates);
    fclose(f2);
    return 11;
  }

  while (fgets(define_line, DEFINE_LEN(qubits), f2) != NULL) {
    if (strcmp(define_line, "\n") == 0 || strcmp(define_line, "\r\n") == 0)
      continue;

    if (strncmp(define_line, "#circ", 5) == 0)
      break;

    if (reg.count >= reg.capacity) {
      size_t new_cap = reg.capacity * 2;
      QuantumGate *tmp = realloc(reg.gates, new_cap * sizeof(QuantumGate));
      if (tmp == NULL) {
        fprintf(stderr, "Errore nella riallocazione del registro dei gate\n");
        free(define_line);
        free(reg.gates);
        fclose(f2);
        return 12;
      }
      reg.gates = tmp;
      reg.capacity = new_cap;
    }

    if (parse_define(define_line, &reg, dim) != 0) {
      free(define_line);
      free(reg.gates);
      fclose(f2);
      return 13;
    }
  }

  free(define_line);
  fclose(f2);
  return 0;
}

static int parse_qubits(char *qubits_line, size_t *n) {
  char *buffer[2];
  char *end_pointer = NULL;
  char *save_pointer;

  buffer[0] = strtok_r(qubits_line, " \r\n", &save_pointer);
  buffer[1] = strtok_r(NULL, " \r\n", &save_pointer);
  if (buffer[0] == NULL || buffer[1] == NULL) {
    fprintf(stderr, "Errore nel parsing dei qubits\n");
    return 1;
  }

  long val = strtol(buffer[1], &end_pointer, 10);
  if (*end_pointer != '\0' || val < 0) {
    fprintf(stderr, "Il numero di qubit non è valido\n");
    return 2;
  }

  *n = (size_t)val;
  return 0;
}

static int parse_init(char *init_line, size_t n, Complex *init) {
  char *buffer[2];
  char *save_pointer;

  buffer[0] = strtok_r(init_line, " ", &save_pointer);
  buffer[1] = strtok_r(NULL, "\n\r", &save_pointer);
  if (buffer[0] == NULL || buffer[1] == NULL) {
    fprintf(stderr, "Errore nel parsing del vettore di init\n");
    return 1;
  }

  if (strcmp(buffer[0], "#init") != 0) {
    fprintf(stderr, "Formato non corretto: atteso #init\n");
    return 2;
  }

  return parse_complex(buffer[1], init, n);
}

static void print_gate(const QuantumGate *g) {
  printf("Gate: %s (%zux%zu)\n", g->name, g->dim, g->dim);
  for (size_t row = 0; row < g->dim; row++) {
    printf("  [ ");
    for (size_t col = 0; col < g->dim; col++) {
      const Complex *c = &g->matrix[row * g->dim + col];
      if (c->imag >= 0)
        printf("%8.5f+i%7.5f ", c->real, c->imag);
      else
        printf("%8.5f-i%7.5f ", c->real, -c->imag);
    }
    printf("]\n");
  }
}

static int parse_define(char *define_line, GateRegistry *reg, size_t n) {
  char *save_pointer;

  char *cmd = strtok_r(define_line, " ", &save_pointer);
  char *name = strtok_r(NULL, " ", &save_pointer);
  char *matrix_str = strtok_r(NULL, "\n\r", &save_pointer);

  if (cmd == NULL || name == NULL || matrix_str == NULL) {
    fprintf(stderr, "Errore nel parsing dei #define\n");
    return 1;
  }

  QuantumGate *g = &reg->gates[reg->count];
  strncpy(g->name, name, 31);
  g->name[31] = '\0';
  g->dim = n;

  size_t total_elements = n * n;
  g->matrix = malloc(total_elements * sizeof(Complex));
  if (g->matrix == NULL)
    return 3;

  if (parse_complex(matrix_str, g->matrix, total_elements) != 0) {
    free(g->matrix);
    return 4;
  }

  print_gate(g);

  reg->count++;
  return 0;
}

// static int parse_circ(char *circ_line, QuantumCircuit *circ, GateRegistry
// *reg);

static int parse_complex(char *complex_line, Complex *array, size_t n) {
  char *save_pointer;
  char *token = strtok_r(complex_line, " ([],)\r\n", &save_pointer);

  for (size_t i = 0; i < n; i++) {
    if (token == NULL) {
      fprintf(stderr,
              "Errore: numero di elementi nel vettore di stato insufficiente "
              "(attesi %zu)\n",
              n);
      return 1;
    }

    double real = 0, imag = 0;
    char *endptr;

    if (*token == '+')
      token++;

    if (*token == 'i') {
      imag = strtod(token + 1, &endptr);
      if (endptr == token + 1)
        imag = 1.0;
    } else if (*token == '-' && *(token + 1) == 'i') {
      imag = strtod(token + 2, &endptr);
      if (endptr == token + 2)
        imag = 1.0;
      imag = -imag;
    } else {
      real = strtod(token, &endptr);
      if (*endptr == '+' || *endptr == '-') {
        char sign = *endptr;
        if (*(endptr + 1) == 'i') {
          char *endptr2;
          imag = strtod(endptr + 2, &endptr2);
          if (endptr2 == endptr + 2)
            imag = 1.0;
          if (sign == '-')
            imag = -imag;
        }
      } else if (*endptr == 'i') {
        imag = real;
        real = 0;
      }
    }

    array[i].real = real;
    array[i].imag = imag;

    token = strtok_r(NULL, " ([],)\r\n", &save_pointer);
  }
  return 0;
}