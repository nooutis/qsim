//
// Created by Charlie Carretta on 29/04/2026.
//

#include "data.h"
#include "memory.h"

#include <complex.h>
#include <stdlib.h>

 void free_reg(const GateRegistry *reg) {
    for (size_t i = 0; i < reg->count; i++) {
        if (reg->gates[i].matrix == NULL) continue;
        free(reg->gates[i].matrix);
    }
    free(reg->gates);
}

void cleanup_circuit(QuantumCircuit *circ) {
     if (circ == NULL) return;
     if (circ->state_vector) free(circ->state_vector);
     for (size_t i = 0; i < circ->num_gates; i++) {
         if (circ->gates == NULL) continue;
         if (circ->gates[i].matrix) free(circ->gates[i].matrix);
     }
     if (circ->gates) free(circ->gates);
     circ->num_gates = 0;
     circ->capacity = 0;
     circ->state_vector = NULL;
     circ->gates = NULL;
 }
