//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_MEASUREMENT_H
#define SO2_MEASUREMENT_H

#include "data.h"

int measure_circuit(const Complex *v_out, size_t num_threads, size_t repetitions, size_t dim, ThreadPool *tm);



#endif //SO2_MEASUREMENT_H