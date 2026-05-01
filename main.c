#include <stdio.h>
#include <unistd.h>

#include "parser.h"

int main(const int argc, const char *argv[]) {
    int thread_number = 1;
    QuantumCircuit circuit;
    parse_main(argc, argv, &thread_number, &circuit);
    return 0;
}