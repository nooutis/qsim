//
// Created by Charlie Carretta on 29/04/2026.
//

#ifndef SO2_DATA_H
#define SO2_DATA_H

// Libreria contenente size_t
#include <stddef.h>

// Macro per ottenere l'indice della matrice
#define GET_INDEX(row, col, dim) (((row) * (dim)) + (col))

// Macro per definire le dimensioni delle matrici
#define DIM(n) ((size_t)1 << n)

// Numeri Complessi
typedef struct complex {
    double real;
    double imag;
} Complex;

//Operazioni sui Numeri Complessi
Complex complex_add(Complex a, Complex b); // Somma di numeri complessi, i due addendi sono a e b
Complex complex_prod(Complex a, Complex b); // Prodotto di numeri complessi, i due moltiplicandi sono a e b
double complex_mod(Complex a); // Modulo di un numero complesso, il numero complesso di cui si calcola il modulo è a

// Operazioni sulle matrici
void matrix_prod(const Complex *a, const Complex *b, Complex *c, const size_t dim); // Prodotto di due matrici, prende in input le due matrici da moltiplicare, la matrice di output su cui si scrive e la dimensione delle due matrici quadrate
void matrix_vector_prod(const Complex *m, const Complex *v_in, Complex *v_out, size_t dim); // Moltiplica una matrice per un vettore, prende in input la matrice da moltiplicare, il vettore da moltiplicare, il vettore di output su cui si scrive e la dimensione della matrice e del vettore



#endif //SO2_DATA_H