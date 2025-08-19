// onebit_predictor.cc
// Simple 1-bit branch predictor template implementation
#include "onebit_predictor.h"
#include <stdlib.h>
#define TAKEN 1
#define NOTTAKEN 0
static uint8_t* table = NULL;
static uint32_t mask = 0;
static int bits = 0;
void onebit_predictor_init(int table_bits) {
    bits = table_bits;
    uint32_t size = 1 << bits;
    table = (uint8_t*)malloc(size * sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++) table[i] = NOTTAKEN;
    mask = size - 1;
}
uint8_t onebit_predictor_predict(uint32_t pc) {
    uint32_t idx = pc & mask;
    return table[idx];
}
void onebit_predictor_train(uint32_t pc, uint8_t outcome) {
    uint32_t idx = pc & mask;
    table[idx] = outcome ? TAKEN : NOTTAKEN;
}
void onebit_predictor_cleanup() {
    if (table) free(table);
    table = NULL;
}
