//
// Created by Charlie Carretta on 29/04/2026.
//

#include "parser.h"
#include <stdlib.h>
#include <string.h>

int parse_main(int argc, const char *argv[], int *thread_number, QuantumCircuit *circ) {

    char *end_pointer = "\0";
    char qubits_line[QUBITS_LEN] = "\0";
    size_t qubits;

    if (argc < 3) {
        fprintf(stderr,"Numero di Parametri Insufficiente");
        return 1;
    }
    FILE *f1 = fopen(argv[1],"r");
    FILE *f2 = fopen(argv[2],"r");

    if (f1 == NULL || f2 == NULL) {
        fprintf(stderr,"Errore nella lettura dei file");
        return 2;
    }

    *thread_number = (int) strtol(argv[3],&end_pointer,10);
    if (*end_pointer != '\0') {
        fprintf(stderr, "Il numero di thread deve essere un intero");
        return 3;
    }


    if (fgets(qubits_line, QUBITS_LEN, f1) == NULL) {
        fprintf(stderr,"Errore nella lettura dei qubits");
    }

    parse_qubits(qubits_line, &qubits);
    circ->qubits = qubits;

    char init_line[INIT_LEN(qubits)];
    fscanf(f1, "%s", init_line);
    // if (init_line[0] )
    if (fgets(init_line, INIT_LEN(qubits), f1) == NULL) {
        fprintf(stderr, "Errore nella lettura del valore iniziale");
    }

    parse_init(init_line, (size_t *) DIM(qubits));
    return 0;
}

int parse_qubits( char *qubits_line, size_t *n) {
    char *buffer[2];
    char *end_pointer = "\0";
    buffer[0] = strtok(qubits_line, " ");
    buffer[1] = strtok(NULL, " ");
    if (buffer[0] == NULL || buffer[1] == NULL) {
        fprintf(stderr,"Errore nel parsing dei qubits");
        return 1;
    }

    *n = strtol(buffer[1], &end_pointer, 10);
    if (*end_pointer != '\0') {
        fprintf(stderr,"Il numero di qubit non è valido");
        return 2;
    }

    if (*n < 0) {
        fprintf(stderr, "Il numero di qubit non può essere negativo");
    }

    return 0;
}

int parse_init( char *init_line, size_t *n) {
    char *buffer[2];
    char *end_pointer = "\0";
    /*buffer[0] = strtok(init_line, " ");
    buffer[1] = strtok(NULL, " ");
    if (buffer[0] == NULL || buffer[1] == NULL) {
        fprintf(stderr,"Errore nel parsing dei qubits");
        return 1;
    }

    *n = strtol(buffer[1], &end_pointer, 10);
    if (*end_pointer != '\0') {
        fprintf(stderr,"Il numero di qubit non è valido");
        return 2;
    }*/

    return 0;
}

//int parse_define(char *define_line, GateRegistry *reg, size_t *n);

//int parse_circ(char *circ_line, QuantumCircuit *circ, GateRegistry *reg);