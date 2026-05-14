#include <stdio.h>

#include "data.h"
#include "memory.h"
#include "parser.h"


int main(const int argc, const char *argv[]) {
  int thread_number = 1;
  QuantumCircuit circuit = {0, NULL, NULL, 0, 0, 0};
  int code = parse_main(argc, argv, &thread_number, &circuit);
  if (code != 0)
    return code;
  Complex *v_out = malloc(sizeof(Complex) * DIM(circuit.qubits));
  multithread(circuit, v_out, thread_number, circuit.repetitions);
  cleanup_circuit(&circuit);
  free(v_out);
  return 0;
}