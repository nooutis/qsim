//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_MEMORY_H
#define SO2_MEMORY_H

#include "data.h"


void free_reg(const GateRegistry *reg);
void cleanup_circuit(QuantumCircuit *circ);
MultiplicationTask allocate_matrix_tasks(size_t num_tasks, size_t mat_size);
int reallocate_matrix_tasks(MultiplicationTask *task, size_t new_num_tasks, size_t mat_size);
void free_matrix_tasks(MultiplicationTask *task);

#endif //SO2_MEMORY_H