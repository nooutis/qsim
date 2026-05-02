#include <stdio.h>
#include <unistd.h>

#include "parser.h"

int main(const int argc, const char *argv[]) {
    int thread_number = 1;
    QuantumCircuit circuit = {0, NULL, NULL, 0, 0, 0};
    parse_main(argc, argv, &thread_number, &circuit);
    return 0;
}