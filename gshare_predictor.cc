// gshare_predictor_fixed.cc
// Clean gshare implementation from scratch
#include "gshare_predictor.h"
#include <stdlib.h>
#include <stdint.h>

// Pattern History Table - 2-bit saturating counters
static uint8_t* pht = NULL;
static uint32_t pht_size = 0;
static uint32_t pht_mask = 0;

// Global history register
static uint32_t global_history = 0;
static uint32_t history_mask = 0;
static int history_bits = 0;

void gshare_predictor_init(int table_bits, int hist_bits) {
    // Clean up any existing state
    if (pht) free(pht);
    
    // Set parameters
    history_bits = hist_bits;
    pht_size = 1 << table_bits;
    pht_mask = pht_size - 1;
    history_mask = (1 << history_bits) - 1;
    
    // Allocate and initialize Pattern History Table
    pht = (uint8_t*)malloc(pht_size * sizeof(uint8_t));
    
    // Initialize all counters to weakly not taken (1)
    for (uint32_t i = 0; i < pht_size; i++) {
        pht[i] = 1;  // WEAK_NOT_TAKEN
    }
    
    // Initialize global history to 0
    global_history = 0;
}

uint8_t gshare_predictor_predict(uint32_t pc) {
    // Compute gshare index: PC XOR global_history
    uint32_t index = (pc ^ global_history) & pht_mask;
    
    // Get 2-bit counter value
    uint8_t counter = pht[index];
    
    // Predict taken if counter >= 2
    return (counter >= 2) ? 1 : 0;
}

void gshare_predictor_train(uint32_t pc, uint8_t outcome) {
    // Compute same gshare index as in predict
    uint32_t index = (pc ^ global_history) & pht_mask;
    
    // Update 2-bit saturating counter
    uint8_t counter = pht[index];
    
    if (outcome) {
        // Branch taken - increment (saturate at 3)
        if (counter < 3) {
            pht[index] = counter + 1;
        }
    } else {
        // Branch not taken - decrement (saturate at 0)  
        if (counter > 0) {
            pht[index] = counter - 1;
        }
    }
    
    // Update global history: shift left and add new outcome
    global_history = ((global_history << 1) | outcome) & history_mask;
}

void gshare_predictor_cleanup() {
    if (pht) {
        free(pht);
        pht = NULL;
    }
    pht_size = 0;
    pht_mask = 0;
    global_history = 0;
    history_mask = 0;
    history_bits = 0;
}
