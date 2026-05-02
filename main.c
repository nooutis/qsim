#include <stdio.h>
#include "memory.h"
#include "parser.h"

void print_circuit(const QuantumCircuit *circ) {
    printf("--- Quantum Circuit Details ---\n");
    printf("Qubits: %zu (Dimension: %zu)\n", circ->qubits, DIM(circ->qubits));

    printf("Initial State Vector:\n  [ ");
    for (size_t i = 0; i < DIM(circ->qubits); i++) {
        printf("%.2f+i%.2f ", circ->state_vector[i].real, circ->state_vector[i].imag);
    }
    printf("]\n");

    printf("Gates in Circuit (%zu):\n", circ->num_gates);
    for (size_t i = 0; i < circ->num_gates; i++) {
        printf("  - %s (%zux%zu)\n", circ->gates[i].name, circ->gates[i].dim, circ->gates[i].dim);
    }

    printf("Repetitions: %zu\n", circ->repetitions);
    printf("-------------------------------\n");
}

int main(const int argc, const char *argv[]) {
    int thread_number = 1;
    QuantumCircuit circuit = {0, NULL, NULL, 0, 0, 0};
    
    int code = parse_main(argc, argv, &thread_number, &circuit);
    if (code == 0) {
        print_circuit(&circuit);
    } else {
        return code;
    }

    cleanup_circuit(&circuit);
    return 0;
}

