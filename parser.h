//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_PARSER_H
#define SO2_PARSER_H
#include <stdio.h>
#include "data.h"

#define QUBITS_LEN 32
#define INIT_LEN(n) (10+ 20*DIM(n))

int parse_main(int argc, const char *argv[], int *thread_number, QuantumCircuit *circ);

int parse_qubits(char *qubits_line, size_t *n);

int parse_init(char *init_line, size_t *n);

int parse_define(char *define_line, GateRegistry *reg, size_t *n);

int parse_circ(char *circ_line, QuantumCircuit *circ, GateRegistry *reg);



#endif //SO2_PARSER_H