// twobit_predictor.cc
// n-bit saturating counter branch predictor implementation
#include "twobit_predictor.h"
#include <stdlib.h>
 #include <cmath>


// 2-bit saturating counter states:
// 00 (0) = Strongly Not Taken
// 01 (1) = Weakly Not Taken  
// 10 (2) = Weakly Taken
// 11 (3) = Strongly Taken

#define STRONG_NOT_TAKEN  0
#define WEAK_NOT_TAKEN    1
#define WEAK_TAKEN        2
#define STRONG_TAKEN      3

static uint8_t* table = NULL;
static uint32_t mask = 0;
static int bits = 0;
static int n_bits = 2; // default to 2 bits if not set
static uint8_t max_val = (1 << n_bits) - 1;
static uint8_t threshold = (1 << (n_bits - 1));

void twobit_predictor_init(int table_bits) {
    bits = table_bits;
    n_bits = 2; // default, can be set externally if needed
    uint32_t size = 1 << bits;
    table = (uint8_t*)malloc(size * sizeof(uint8_t));
    // Initialize to weakly not taken (neutral, can adapt either direction quickly)
    for (uint32_t i = 0; i < size; i++) table[i] = WEAK_NOT_TAKEN;
    mask = size - 1;
    max_val = (1 << n_bits) - 1;
    threshold = (1 << (n_bits - 1));
}

uint8_t twobit_predictor_predict(uint32_t pc) {
    uint32_t idx = pc & mask;
    uint8_t counter = table[idx];
    // Predict taken if counter >= threshold (upper half of range)
    return (counter >= threshold) ? 1 : 0;
}

void twobit_predictor_train(uint32_t pc, uint8_t outcome) {
    uint32_t idx = pc & mask;
    uint8_t counter = table[idx];
    if (outcome) {
        // Branch was taken - increment counter (saturate at max_val)
        if (counter < max_val) {
            table[idx] = counter + 1;
        }
    } else {
        // Branch was not taken - decrement counter (saturate at 0)
        if (counter > 0) {
            table[idx] = counter - 1;
        }
    }
}

void twobit_predictor_cleanup() {
    if (table) free(table);
    table = NULL;
}
