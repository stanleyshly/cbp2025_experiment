// correlating_predictor.cc
// n-bit correlating (global history) branch predictor implementation
#include "correlating_predictor.h"
#include <stdlib.h>
#include <stdint.h>

static uint8_t* table = NULL;
static uint32_t mask = 0;
static int bits = 0;
static int n_bits = 2;
static uint8_t max_val = 3;
static uint8_t threshold = 2;
static int history_bits = 0;
static uint32_t history = 0;

void correlating_predictor_init(int table_bits, int hist_bits, int nbits) {
    bits = table_bits;
    history_bits = hist_bits;
    n_bits = nbits;
    uint32_t size = 1 << (bits + history_bits);
    table = (uint8_t*)malloc(size * sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++) table[i] = 0;
    mask = size - 1;
    max_val = (1 << n_bits) - 1;
    threshold = (1 << (n_bits - 1));
    history = 0;
}

uint8_t correlating_predictor_predict(uint32_t pc) {
    // Mask to extract the lower 'bits' bits of the PC
    uint32_t pc_mask = (1 << bits) - 1;
    // Mask to extract the lower 'history_bits' bits of the global history
    uint32_t hist_mask = (1 << history_bits) - 1;
    // Use only the lower 'bits' bits of PC, then shift to make room for history
    uint32_t pc_part = (pc & pc_mask) << history_bits;
    // Use only the lower 'history_bits' bits of the global history
    uint32_t hist_part = history & hist_mask;
    uint32_t idx = pc_part | hist_part;
    uint8_t counter = table[idx];
    return (counter >= threshold) ? 1 : 0;
}

void correlating_predictor_train(uint32_t pc, uint8_t outcome) {
    // Mask to extract the lower 'bits' bits of the PC
    uint32_t pc_mask = (1 << bits) - 1;
    // Mask to extract the lower 'history_bits' bits of the global history
    uint32_t hist_mask = (1 << history_bits) - 1;
    // Use only the lower 'bits' bits of PC, then shift to make room for history
    uint32_t pc_part = (pc & pc_mask) << history_bits;
    // Use only the lower 'history_bits' bits of the global history
    uint32_t hist_part = history & hist_mask;
    uint32_t idx = pc_part | hist_part;
    uint8_t counter = table[idx];
    if (outcome) {
        if (counter < max_val) {
            table[idx] = counter + 1;
        }
    } else {
        if (counter > 0) {
            table[idx] = counter - 1;
        }
    }
    // Update global history
    // Update global history: shift left, add new outcome, and mask to keep only 'history_bits' bits
    history = ((history << 1) | (outcome ? 1 : 0)) & ((1 << history_bits) - 1);
}

void correlating_predictor_cleanup() {
    if (table) free(table);
    table = NULL;
}
