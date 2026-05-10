#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "memory.h"
#include "parser.h"
#include "thread.h"

static void print_circuit(const QuantumCircuit *circ) {
  printf("--- Quantum Circuit Details ---\n");
  printf("Qubits: %zu (Dimension: %zu)\n", circ->qubits, DIM(circ->qubits));

  printf("Initial State Vector:\n  [ ");
  for (size_t i = 0; i < DIM(circ->qubits); i++) {
    printf("%.2f+i%.2f ", circ->state_vector[i].real,
           circ->state_vector[i].imag);
  }
  printf("]\n");

  printf("Gates in Circuit (%zu):\n", circ->num_gates);
  for (size_t i = 0; i < circ->num_gates; i++) {
    printf("  - %s (%zux%zu)\n", circ->gates[i].name, circ->gates[i].dim,
           circ->gates[i].dim);
  }

  printf("Repetitions: %zu\n", circ->repetitions);
  printf("-------------------------------\n");
}

void print_matrix(const Complex *matrix, int size) {
  if (matrix == NULL || size <= 0) {
    printf("Error: Invalid matrix pointer or size.\n");
    return;
  }

  printf("\n--- Complex %d x %d Matrix ---\n", size, size);

  // Iterate through each row (i)
  for (int i = 0; i < size; i++) {
    // Start the row block: (
    printf("(");

    // Iterate through each column (j) within the current row
    for (int j = 0; j < size; j++) {
      // Calculate the 1D index for the element A[i][j]
      int index = i * size + j;

      // Print the complex number (real + imag * i) followed by a space
      printf(" %.4f + %.4fi", matrix[index].real, matrix[index].imag);
    }

    // End the row block: )
    printf(");\n");

    // Print the row separator if it's not the last row
    if (i < size - 1) {
      printf("\n"); // Add an extra newline for clarity between rows
    }
  }
}

static void print_vector(const Complex *vector, size_t size) {

  printf("\n--- Complex Vector (%zu elements) ---\n", size);

  // Iterate through each element of the vector
  for (size_t i = 0; i < size; i++) {
    // Calculate the index in the 1D array
    size_t index = i;

    // Print the complex number (real + imag * i) followed by a space
    printf(" %.4f + %.4fi", vector[index].real, vector[index].imag);

    // Add a separator (semicolon and newline for readability)
    if (i < size - 1) {
      printf("\n");
    }
  }

  // End the main block
  printf("\n");
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
  Complex *v_out = malloc(sizeof(Complex)*DIM(circuit.qubits));
  if (thread_number == 1) {
   monothread(circuit, v_out);
    print_vector(v_out, DIM(circuit.qubits));
  }
  if (thread_number > 2) {
    multithread(circuit, v_out, thread_number);
    print_vector(v_out, DIM(circuit.qubits));
  }
  cleanup_circuit(&circuit);
  return 0;
}