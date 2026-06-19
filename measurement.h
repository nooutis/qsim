// File measurement.h, contiene la funzione di misura del circuito

#ifndef SO2_MEASUREMENT_H
#define SO2_MEASUREMENT_H

#include "data.h"

int measure_circuit(const Complex *v_out, size_t num_threads, size_t repetitions, size_t dim, ThreadPool *tm);

#endif //SO2_MEASUREMENT_H