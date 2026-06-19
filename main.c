// File main.c, contiene la funzione principale del programma

#include <stdio.h>

#include "data.h"
#include "memory.h"
#include "parser.h"

/**
 * La funzione principale del programma
 * @param argc Numero di argomenti in input
 * @param argv Vettore degli input del programma
 * @return 0 se non ci sono errori, il codice errore altrimenti
 */
int main(const int argc, const char *argv[]) {
  int num_threads = 1;
  QuantumCircuit circ = {0, NULL, NULL, 0, 0, 0};

  // Parsing dei file di input
  int code = parse_main(argc, argv, &num_threads, &circ);
  if (code != 0)
    return code;

  // Allocazione vettore di output
  Complex *v_out = malloc(sizeof(Complex) * DIM(circ.qubits));
  if (v_out == NULL) {
    cleanup_circuit(&circ);
    return 1;
  }

  // Esecuzione simulazione (Evoluzione + Misurazione)
  multithread(circ, v_out, num_threads);

  // Pulizia
  cleanup_circuit(&circ);
  free(v_out);
  return 0;
}