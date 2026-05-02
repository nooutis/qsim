//
// Created by Charlie Carretta on 29/04/2026.
//

#include "parser.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int parse_qubits(char *qubits_line, size_t *n);
static int parse_init(char *init_line, size_t n, Complex *init);
static int parse_define(char *define_line, GateRegistry *reg, size_t n);
static int parse_circ(char *circ_line, QuantumCircuit *circ, const GateRegistry *reg);
static int parse_complex(char *complex_line, Complex *array, size_t n);
static QuantumGate* find_gate(const GateRegistry *reg, const char *name);
static int add_gate(QuantumCircuit *circ, const QuantumGate *gate);



int parse_main(int argc, const char *argv[], int *thread_number, QuantumCircuit *circ) {
  char *endptr = NULL;
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

  *thread_number = (int)strtol(argv[3], &endptr, 10);
  if (*endptr != '\0' || *thread_number <= 0) {
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

  {
    size_t init_len = strlen(init_line);
    while (strchr(init_line, ']') == NULL && init_len < INIT_LEN(qubits) - 1) {
      if (fgets(init_line + init_len, INIT_LEN(qubits) - init_len, f1) == NULL)
        break;
      init_len += strlen(init_line + init_len);
    }
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

  GateRegistry reg = {NULL, 0, 4};
  reg.gates = malloc(reg.capacity * sizeof(QuantumGate));
  if (reg.gates == NULL) {
    fprintf(stderr, "Errore di allocazione del registro dei gate\n");
    fclose(f2);
    return 10;
  }

  char *define_line = malloc(DEFINE_LEN(qubits));
  if (define_line == NULL) {
    fprintf(stderr, "Errore di allocazione per define_line\n");
    free_reg(&reg);
    fclose(f2);
    return 11;
  }

  while (fgets(define_line, DEFINE_LEN(qubits), f2) != NULL) {
    if (strcmp(define_line, "\n") == 0 || strcmp(define_line, "\r\n") == 0)
      continue;

    if (strncmp(define_line, "#circ", 5) == 0)
      break;

    if (strncmp(define_line, "#define", 7) == 0) {
      size_t def_len = strlen(define_line);
      while (strchr(define_line, ']') == NULL && def_len < DEFINE_LEN(qubits) - 1) {
        if (fgets(define_line + def_len, DEFINE_LEN(qubits) - def_len, f2) == NULL)
          break;
        def_len += strlen(define_line + def_len);
      }
    }

    if (reg.count >= reg.capacity) {
      size_t new_cap = reg.capacity * 2;
      QuantumGate *tmp = realloc(reg.gates, new_cap * sizeof(QuantumGate));
      if (tmp == NULL) {
        fprintf(stderr, "Errore nella riallocazione del registro dei gate\n");
        free(define_line);
        free_reg(&reg);
        fclose(f2);
        return 12;
      }
      reg.gates = tmp;
      reg.capacity = new_cap;
    }

    if (parse_define(define_line, &reg, dim) != 0) {
      free(define_line);
      free_reg(&reg);
      fclose(f2);
      return 13;
    }
  }

  if (strncmp(define_line, "#circ", 5) == 0) {
    if (parse_circ(define_line, circ, &reg) != 0) {
      free_reg(&reg);
      free(define_line);
      return 14;
    }
  }

  free_reg(&reg);
  free(define_line);
  fclose(f2);
  return 0;
}

static int parse_qubits(char *qubits_line, size_t *n) {
  char *buffer[2];
  char *endptr = NULL;
  char *save_pointer;

  buffer[0] = strtok_r(qubits_line, " \r\n", &save_pointer);
  buffer[1] = strtok_r(NULL, " \r\n", &save_pointer);
  if (buffer[0] == NULL || buffer[1] == NULL) {
    fprintf(stderr, "Errore nel parsing dei qubits\n");
    return 1;
  }

  long val = strtol(buffer[1], &endptr, 10);
  if (*endptr != '\0' || val < 0) {
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
  buffer[1] = strtok_r(NULL, "]", &save_pointer);
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
  char *matrix_str = strtok_r(NULL, "]", &save_pointer);

  if (cmd == NULL || name == NULL || matrix_str == NULL) {
    fprintf(stderr, "Errore nel parsing dei #define\n");
    return 1;
  }

  QuantumGate *gates = &reg->gates[reg->count];
  strncpy(gates->name, name, 31);
  gates->name[31] = '\0';
  gates->dim = n;

  size_t total_elements = n * n;
  gates->matrix = malloc(total_elements * sizeof(Complex));
  if (gates->matrix == NULL)
    return 3;

  if (parse_complex(matrix_str, gates->matrix, total_elements) != 0) {
    free(gates->matrix);
    return 4;
  }

  print_gate(gates);

  reg->count++;
  return 0;
}

static int parse_circ(char *circ_line, QuantumCircuit *circ, const GateRegistry *reg) {
  char *save_pointer;
  char *token;

  strtok_r(circ_line, " ", &save_pointer); // Skip "#circ"

  while ((token = strtok_r(NULL, " \t\n\r", &save_pointer)) != NULL) {
    if (strcmp(token, "measure") == 0) {
      token = strtok_r(NULL, " \t\n\r", &save_pointer);
      if (token == NULL) {
        fprintf(stderr, "Errore: Inserire il numero di ripetizioni\n");
        return 2;
      }
      char *endptr;
      circ->repetitions = strtol(token, &endptr, 10);
      return (*endptr == '\0') ? 0 : 4;
    }

    QuantumGate *gate = find_gate(reg, token);
    if (gate == NULL) {
      // Try if it's a numeric repetition without "measure"
      char *endptr;
      long reps = strtol(token, &endptr, 10);
      if (*endptr == '\0' && reps >= 0) {
        circ->repetitions = (size_t)reps;
        return 0;
      }
      fprintf(stderr, "Porta non trovata: %s\n", token);
      return 3;
    }

    if (add_gate(circ, gate) != 0) return 4;
  }
  return 0;
}

static int parse_complex(char *complex_line, Complex *array, size_t n) {
  char *save_pointer;
  char *token = strtok_r(complex_line, " ([],)\r\n", &save_pointer);

  for (size_t i = 0; i < n; i++) {
    if (token == NULL) {
      fprintf(stderr, "Errore: numero di elementi insufficiente (attesi %zu)\n", n);
      return 1;
    }

    double real = 0, imag = 0;
    char *endptr, *endptr2;

    real = strtod(token, &endptr);
    if (*endptr == '+' || *endptr == '-') {
      char sign = *endptr;
      char *next = endptr + 1;
      if (*next == 'i') {
        imag = strtod(next + 1, &endptr2);
        if (endptr2 == next + 1) imag = 1.0;
      } else {
        imag = strtod(next, &endptr2);
      }
      if (sign == '-') imag = -imag;
    }

    array[i].real = real;
    array[i].imag = imag;
    token = strtok_r(NULL, " ([]),\r\n ", &save_pointer);
  }
  return 0;
}

static QuantumGate* find_gate(const GateRegistry *reg, const char *name) {
  for (size_t i = 0; i < reg->count; i++) {
    if (strcmp(reg->gates[i].name, name) == 0) {
      return &reg->gates[i];
    }
  }
  return NULL;
}

static int add_gate(QuantumCircuit *circ, const QuantumGate *gate) {
  if (circ->num_gates >= circ->capacity) {
    size_t new_cap = (circ->capacity == 0) ? 4 : circ->capacity * 2;
    QuantumGate *tmp = realloc(circ->gates, new_cap * sizeof(QuantumGate));
    if (tmp == NULL) {
      fprintf(stderr, "Errore di allocazione per i gate del circuito\n");
      return 1;
    }
    circ->gates = tmp;
    circ->capacity = new_cap;
  }

  QuantumGate *new_gate = &circ->gates[circ->num_gates];
  strncpy(new_gate->name, gate->name, 31);
  new_gate->name[31] = '\0';
  new_gate->dim = gate->dim;

  size_t total_elements = gate->dim * gate->dim;
  new_gate->matrix = malloc(total_elements * sizeof(Complex));
  if (new_gate->matrix == NULL) {
    fprintf(stderr, "Errore di allocazione per la matrice del gate nel circuito\n");
    return 2;
  }

  memcpy(new_gate->matrix, gate->matrix, total_elements * sizeof(Complex));

  circ->num_gates++;
  return 0;
}

