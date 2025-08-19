// local_predictor.cc
// Local (two-level) branch predictor implementation
#include "local_predictor.h"
#include <stdlib.h>
#include <stdint.h>

// Table 1: Local History Table (LHT) - stores local history for each branch
static uint32_t* lht = NULL;
static uint32_t lht_mask = 0;
static int lht_bits = 0;

// Table 2: Pattern History Table (PHT) - stores 2-bit saturating counters
static uint8_t* pht = NULL;
static uint32_t pht_mask = 0;
static int pht_bits = 0;

// History configuration
static int history_bits = 0;
static uint32_t history_mask = 0;

// 2-bit saturating counter states
static const uint8_t STRONG_NOT_TAKEN = 0;
static const uint8_t WEAK_NOT_TAKEN = 1;
static const uint8_t WEAK_TAKEN = 2;
static const uint8_t STRONG_TAKEN = 3;

void local_predictor_init(int _lht_bits, int _history_bits, int _pht_bits) {
    lht_bits = _lht_bits;
    history_bits = _history_bits;
    pht_bits = _pht_bits;
    
    // Local History Table: 2^lht_bits entries, each storing history_bits of history
    uint32_t lht_size = 1 << lht_bits;
    lht = (uint32_t*)malloc(lht_size * sizeof(uint32_t));
    for (uint32_t i = 0; i < lht_size; i++) lht[i] = 0;
    lht_mask = lht_size - 1;
    
    // Pattern History Table: 2^pht_bits entries, each storing 2-bit counters
    uint32_t pht_size = 1 << pht_bits;
    pht = (uint8_t*)malloc(pht_size * sizeof(uint8_t));
    for (uint32_t i = 0; i < pht_size; i++) pht[i] = WEAK_NOT_TAKEN; // Initialize to weakly not taken
    pht_mask = pht_size - 1;
    
    history_mask = (1 << history_bits) - 1;
}

uint8_t local_predictor_predict(uint32_t pc) {
    // Step 1: Index into LHT using low bits of PC
    uint32_t lht_idx = pc & lht_mask;
    uint32_t local_history = lht[lht_idx] & history_mask;
    
    // Step 2: Index into PHT using local history
    uint32_t pht_idx = local_history & pht_mask;
    uint8_t counter = pht[pht_idx];
    
    // Predict taken if counter >= 2 (WEAK_TAKEN or STRONG_TAKEN)
    return (counter >= 2) ? 1 : 0;
}

void local_predictor_train(uint32_t pc, uint8_t outcome) {
    // Step 1: Index into LHT using low bits of PC
    uint32_t lht_idx = pc & lht_mask;
    uint32_t local_history = lht[lht_idx] & history_mask;
    
    // Step 2: Index into PHT using local history and update counter
    uint32_t pht_idx = local_history & pht_mask;
    uint8_t counter = pht[pht_idx];
    
    if (outcome) {
        // Branch was taken - increment counter (saturate at 3)
        if (counter < STRONG_TAKEN) {
            pht[pht_idx] = counter + 1;
        }
    } else {
        // Branch was not taken - decrement counter (saturate at 0)
        if (counter > STRONG_NOT_TAKEN) {
            pht[pht_idx] = counter - 1;
        }
    }
    
    // Step 3: Update local history in LHT
    // Shift left, add new outcome, and mask to keep only history_bits
    lht[lht_idx] = ((local_history << 1) | (outcome ? 1 : 0)) & history_mask;
}

void local_predictor_cleanup() {
    if (lht) free(lht);
    if (pht) free(pht);
    lht = NULL;
    pht = NULL;
}
