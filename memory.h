//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_MEMORY_H
#define SO2_MEMORY_H

#include "data.h"

void free_reg(const GateRegistry *reg);
void cleanup_circuit(QuantumCircuit *circ);

#endif //SO2_MEMORY_H