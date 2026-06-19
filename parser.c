/* File parser.c, contiene le funzioni di parsing che permettono di trasformare
 * i file di input in una struttura QuantumCircuit
 */

#include "parser.h"
#include "memory.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_whitespace_only(const char *str);
static char *skip_leading_whitespace(char *str);
static int parse_qubits(char *qubits_line, size_t *n);
static int parse_init(char *init_line, size_t dim, Complex *init);
static int parse_define(char *define_line, GateRegistry *reg, size_t n);
static int parse_circ(char *circ_line, QuantumCircuit *circ,
                      const GateRegistry *reg);
static int parse_complex(char *complex_line, Complex *array, size_t n);
static QuantumGate *find_gate(const GateRegistry *reg, const char *name);
static int add_gate(QuantumCircuit *circ, const QuantumGate *gate);

/**
 * Verifica se la stringa contiene solo whitespaces
 * @param str La stringa da verificare
 * @return 0 se la stringa non contiene solo whitespaces, 1 altrimenti
 */
static int is_whitespace_only(const char *str) {
  while (*str) {
    if (!isspace((unsigned char)*str)) {
      return 0;
    }
    str++;
  }
  return 1;
}

/**
 * Elimina gli spazi iniziali della stringa
 * @param str La stringa da cui rimuovere gli spazi
 * @return Il puntatore alla stringa senza gli spazi iniziali
 */
static char *skip_leading_whitespace(char *str) {
  while (*str && isspace((unsigned char)*str)) {
    str++;
  }
  return str;
}

/**
 * Funzione principale del parsing. Legge i due file di input e popola la
 * struttura QuantumCircuit.
 * @param argc Numero di argomenti della riga di comando.
 * @param argv Vettore degli argomenti.
 * @param num_threads Puntatore dove salvare il numero di thread.
 * @param circ Puntatore alla struttura del circuito da popolare.
 * @return 0 in caso di successo, altrimenti codice di errore.
 */
int parse_main(int argc, const char *argv[], int *num_threads,
               QuantumCircuit *circ) {
  char *endptr = NULL;
  char qubits_line[QUBITS_LEN];
  size_t qubits;

  // Controllo numero parametri
  if (argc < 4) {
    fprintf(stderr,
            "Numero di Parametri Insufficiente. Uso: %s <file1> <file2> "
            "<n_thread>\n",
            argv[0]);
    return 1;
  }

  // Apertura file inizializzazione e circuito
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

  // Parsing numero thread
  *num_threads = (int)strtol(argv[3], &endptr, 10);
  if (*endptr != '\0' || *num_threads <= 0) {
    fprintf(stderr, "Il numero di thread deve essere un intero positivo\n");
    fclose(f1);
    fclose(f2);
    return 3;
  }

  // Lettura numero qubit
  do {
    if (fgets(qubits_line, QUBITS_LEN, f1) == NULL) {
      fprintf(stderr, "Errore nella lettura dei qubits\n");
      fclose(f1);
      fclose(f2);
      return 4;
    }
  } while (is_whitespace_only(qubits_line));

  if (parse_qubits(qubits_line, &qubits) != 0) {
    fclose(f1);
    fclose(f2);
    return 5;
  }

  circ->qubits = qubits;
  size_t dim = DIM(qubits);

  // Lettura e parsing dello stato iniziale (#init)
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
  } while (is_whitespace_only(init_line));

  circ->state_vector = malloc(dim * sizeof(Complex));
  if (circ->state_vector == NULL) {
    fprintf(stderr, "Errore di allocazione per state_vector\n");
    free(init_line);
    fclose(f1);
    fclose(f2);
    return 8;
  }

  // Gestione righe multiple per #init
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

  // Registro per memorizzare le definizioni delle porte (#define)
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

  // Parsing delle definizioni e della sequenza del circuito
  while (fgets(define_line, DEFINE_LEN(qubits), f2) != NULL) {
    char *trimmed = skip_leading_whitespace(define_line);
    if (*trimmed == '\0')
      continue;
    memmove(define_line, trimmed, strlen(trimmed) + 1);

    if (strncmp(define_line, "#circ", 5) == 0)
      break;

    if (strncmp(define_line, "#define", 7) == 0) {
      size_t def_len = strlen(define_line);
      while (strchr(define_line, ']') == NULL &&
             def_len < DEFINE_LEN(qubits) - 1) {
        if (fgets(define_line + def_len, DEFINE_LEN(qubits) - def_len, f2) ==
            NULL)
          break;
        def_len += strlen(define_line + def_len);
      }
      trimmed = skip_leading_whitespace(define_line);
      memmove(define_line, trimmed, strlen(trimmed) + 1);
    }

    // Ridimensionamento del registro dei gate se necessario
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

  // Parsing finale della sequenza di porte (#circ)
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

/**
 * Parsa la riga contenente il numero di qubit.
 * @param qubits_line La riga contenente il numero di qubit
 * @param num_qubits Il puntatore al numero di qubit
 * @return 0 se il parsing è riuscito, un intero positivo in caso di errore
 */
static int parse_qubits(char *qubits_line, size_t *num_qubits) {
  char *buffer[2];
  char *endptr = NULL;
  char *save_pointer;

  // Parsing della riga contenente il numero di qubit
  buffer[0] = strtok_r(qubits_line, " \t\r\n", &save_pointer);
  buffer[1] = strtok_r(NULL, " \t\r\n", &save_pointer);
  if (buffer[0] == NULL || buffer[1] == NULL) {
    fprintf(stderr, "Errore nel parsing dei qubits\n");
    return 1;
  }

  long val = strtol(buffer[1], &endptr, 10);
  if (*endptr != '\0' || val < 0) {
    fprintf(stderr, "Il numero di qubit non è valido\n");
    return 2;
  }

  *num_qubits = (size_t)val;
  return 0;
}

/**
 * Parsa il vettore di stato iniziale
 * @param init_line La riga contenente il vettore di stato
 * @param dim Dimensioni del vettore di stato
 * @param init Vettore di stato
 * @return 0 se il parsing è riuscito, 1 altrimenti
 */
static int parse_init(char *init_line, size_t dim, Complex *init) {
  char *buffer[2];
  char *save_pointer;

  buffer[0] = strtok_r(init_line, " \t\r\n", &save_pointer);
  buffer[1] = strtok_r(NULL, "]", &save_pointer);
  if (buffer[0] == NULL || buffer[1] == NULL) {
    fprintf(stderr, "Errore nel parsing del vettore di init\n");
    return 1;
  }

  if (strcmp(buffer[0], "#init") != 0) {
    fprintf(stderr, "Formato non corretto: atteso #init\n");
    return 2;
  }

  return parse_complex(buffer[1], init, dim);
}

/**
 * Parsa la definizione di una porta quantistica (#define).
 * @param define_line La riga contenente la matrice da definire
 * @param reg Il GateRegistry in cui salvare le informazioni delle matrici
 * @param n Dimensioni della matrice
 * @return 0 se il parsing è riuscito, un numero intero positivo altrimenti
 */
static int parse_define(char *define_line, GateRegistry *reg, size_t n) {
  char *save_pointer;

  // Salva cmd e name
  char *cmd = strtok_r(define_line, " \t\r\n", &save_pointer);
  char *name = strtok_r(NULL, " \t\r\n", &save_pointer);
  char *matrix_str = strtok_r(NULL, "]", &save_pointer);

  // Verifica che i vettori cmd, name e matrix_str siano validi
  if (cmd == NULL || name == NULL || matrix_str == NULL) {
    fprintf(stderr, "Errore nel parsing dei #define\n");
    return 1;
  }

  // Salva il nome della matrice
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

  reg->count++;
  return 0;
}

/**
 * Parsa la riga #circ per costruire la sequenza di porte del circuito.
 * @param circ_line La riga contenente la definizione del circuito
 * @param circ Il QuantumCircuit in cui salvare le informazioni del circuito
 * @param reg Il GateRegistry da cui sono prelevate le informazioni sulle porte
 * logiche
 * @return 0 in caso il parsing sia riuscito, un numero intero positivo
 * altrimenti
 */
static int parse_circ(char *circ_line, QuantumCircuit *circ,
                      const GateRegistry *reg) {
  char *save_pointer;
  char *token;

  strtok_r(circ_line, " \t\r\n", &save_pointer); // Salta "#circ"

  while ((token = strtok_r(NULL, " \t\n\r", &save_pointer)) != NULL) {
    // Gestione del comando 'measure'
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

    // Ricerca della porta nel registro
    QuantumGate *gate = find_gate(reg, token);
    if (gate == NULL) {
      // Gestione del numero di ripetizioni (formato legacy)
      char *endptr;
      long reps = strtol(token, &endptr, 10);
      if (*endptr == '\0' && reps >= 0) {
        circ->repetitions = (size_t)reps;
        return 0;
      }
      fprintf(stderr, "Porta non trovata: %s\n", token);
      return 3;
    }

    if (add_gate(circ, gate) != 0)
      return 4;
  }
  return 0;
}

/**
 * Converte una stringa di numeri complessi in un array di strutture Complex.
 * @param complex_line La stringa da cui estrarre l'array complesso
 * @param array L'array complesso in cui salvare il risultato
 * @param n La dimensione dell'array
 * @return 0 in caso il parsing sia riuscito, 1 in caso di errore
 */
static int parse_complex(char *complex_line, Complex *array, size_t n) {
  char *save_pointer;
  char *token = strtok_r(complex_line, " \t([],)\r\n", &save_pointer);

  for (size_t i = 0; i < n; i++) {
    if (token == NULL) {
      fprintf(stderr, "Errore: numero di elementi insufficiente (attesi %zu)\n",
              n);
      return 1;
    }

    double real = 0, imag = 0;
    char *endptr, *endptr2;

    // Parsing parte reale
    real = strtod(token, &endptr);
    // Parsing eventuale parte immaginaria (+i... o -i... o i...)
    if (*endptr == '+' || *endptr == '-' || *endptr == 'i') {
      char sign = '+';
      char *next = endptr;
      if (*endptr == '+' || *endptr == '-') {
        sign = *endptr;
        next = endptr + 1;
      }
      if (*next == 'i') {
        imag = strtod(next + 1, &endptr2);
        if (endptr2 == next + 1)
          imag = 1.0;
      } else {
        imag = strtod(next, &endptr2);
      }
      if (sign == '-')
        imag = -imag;
    }

    array[i].real = real;
    array[i].imag = imag;
    token = strtok_r(NULL, " \t([],)\r\n", &save_pointer);
  }
  return 0;
}

/**
 * Cerca una porta per nome nel registro.
 * @param reg Il GateRegistry in cui cercare le porte
 * @param name Il nome della porta da cercare
 * @return Il puntatore alla porta logica se trovata, NULL altrimenti
 */
static QuantumGate *find_gate(const GateRegistry *reg, const char *name) {
  for (size_t i = 0; i < reg->count; i++) {
    if (strcmp(reg->gates[i].name, name) == 0) {
      return &reg->gates[i];
    }
  }
  return NULL;
}

/**
 * Aggiunge una porta alla sequenza del circuito.
 * @param circ Il circuito a cui aggiungere la porta
 * @param gate La porta da aggiungere
 * @return 0 in caso di successo, un numero intero positivo altrimenti
 */
static int add_gate(QuantumCircuit *circ, const QuantumGate *gate) {
  // Espansione del QuantumCircuit se necessario
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
    fprintf(stderr,
            "Errore di allocazione per la matrice del gate nel circuito\n");
    return 2;
  }

  memcpy(new_gate->matrix, gate->matrix, total_elements * sizeof(Complex));

  circ->num_gates++;
  return 0;
}
