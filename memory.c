//
// Created by Charlie Carretta on 29/04/2026.
//

#include "data.h"
#include "memory.h"

#include <stdlib.h>
#include <_stdio.h>

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

MultiplicationTask allocate_matrix_tasks(size_t num_tasks, size_t mat_size) {
     MultiplicationTask allocated_task = {NULL, num_tasks};
     allocated_task.matrix = malloc(sizeof(ComplexMatrix)*num_tasks);
     for (size_t i = 0; i < num_tasks; i++) {
         allocated_task.matrix[i].matrix = malloc(sizeof(Complex)*mat_size);
     }
     return allocated_task;
 }

int reallocate_matrix_tasks(MultiplicationTask *task, size_t new_num_tasks, size_t mat_size) {
     if (new_num_tasks > task->num_tasks) {
         ComplexMatrix *new_task = realloc(task->matrix, sizeof(ComplexMatrix)*new_num_tasks);
         if (new_task == NULL) {
             fprintf(stderr, "Errore nella riallocazione della matrice");
             return 1;
         }
         task->matrix = new_task;
         for (size_t i = task->num_tasks; i < new_num_tasks; i++) {
             task->matrix[i].matrix = malloc(sizeof(Complex)*mat_size);
         }
     }
    if (new_num_tasks < task->num_tasks) {
        for (size_t i = new_num_tasks; i < task->num_tasks; i++) {
            free(task->matrix[i].matrix);
        }
        ComplexMatrix *new_task = realloc(task->matrix, sizeof(ComplexMatrix)*new_num_tasks);
        if (new_task == NULL) {
            fprintf(stderr, "Errore nella riallocazione della matrice");
            return 1;
        }
        task->matrix = new_task;
    }
    return 0;
 }

void free_matrix_tasks(MultiplicationTask *task) {
     for (size_t i = 0; i < task->num_tasks; i++) {
         free(task->matrix[i].matrix);
     }
     free(task->matrix);
 }
