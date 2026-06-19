// File parser.h, contiene la funzione principale di parsing

#ifndef SO2_PARSER_H
#define SO2_PARSER_H

#include "data.h"

#define QUBITS_LEN    32
#define INIT_LEN(n)   (10 + 30*DIM(n))
#define DEFINE_LEN(n) (32 + 30*DIM(n)*DIM(n))
#define WORD_LEN      8

int parse_main(int argc, const char *argv[], int *thread_number, QuantumCircuit *circ);

#endif // SO2_PARSER_H