//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_PARSER_H
#define SO2_PARSER_H
#include "data.h"
#include <stdio.h>

#define QUBITS_LEN 32
#define INIT_LEN(n) (10 + 20 * DIM(n))
#define DEFINE_LEN(n) (32 + 20 * DIM(n) * DIM(n))

int parse_main(int argc, const char *argv[], int *thread_number,
               QuantumCircuit *circ);

static int parse_qubits(char *qubits_line, size_t *n);

static int parse_init(char *init_line, size_t n, Complex *init);

static int parse_define(char *define_line, GateRegistry *reg, size_t n);

static int parse_circ(char *circ_line, QuantumCircuit *circ, GateRegistry *reg);

static int parse_complex(char *complex_line, Complex *array, size_t n);

#endif // SO2_PARSER_H