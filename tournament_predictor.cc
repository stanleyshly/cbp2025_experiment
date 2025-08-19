// tournament_predictor.cc
// Tournament branch predictor that selects between bimodal and gshare predictors
#include "tournament_predictor.h"
#include "twobit_predictor.h"
#include "gshare_predictor.h"
#include <stdlib.h>

// Tournament selector states (2-bit saturating counter)
#define STRONG_BIMODAL    0  // Strongly prefer bimodal (P1)
#define WEAK_BIMODAL      1  // Weakly prefer bimodal (P1)  
#define WEAK_GSHARE       2  // Weakly prefer gshare (P2)
#define STRONG_GSHARE     3  // Strongly prefer gshare (P2)

// Tournament predictor state
static uint8_t* selector_table = NULL;
static uint32_t selector_mask = 0;
static int selector_bits = 0;
static bool initialized = false;

// Store predictions for training
static uint8_t last_bimodal_pred = 0;
static uint8_t last_gshare_pred = 0;
static uint32_t last_pc = 0;

void tournament_predictor_init(int _selector_bits, int bimodal_bits, int gshare_table_bits, int gshare_history_bits) {
    // Clean up any existing state
    tournament_predictor_cleanup();
    
    // Initialize selector table
    selector_bits = _selector_bits;
    uint32_t selector_size = 1 << selector_bits;
    selector_table = (uint8_t*)malloc(selector_size * sizeof(uint8_t));
    
    // Initialize selectors to weakly prefer bimodal (historical default)
    for (uint32_t i = 0; i < selector_size; i++) {
        selector_table[i] = WEAK_BIMODAL;
    }
    selector_mask = selector_size - 1;
    
    // Initialize the two component predictors
    twobit_predictor_init(bimodal_bits);      // P1: Bimodal predictor
    gshare_predictor_init(gshare_table_bits, gshare_history_bits);  // P2: Gshare predictor
    
    initialized = true;
}

uint8_t tournament_predictor_predict(uint32_t pc) {
    if (!initialized) return 0;
    
    // Get predictions from both component predictors
    uint8_t bimodal_pred = twobit_predictor_predict(pc);
    uint8_t gshare_pred = gshare_predictor_predict(pc);
    
    // Store predictions for training
    last_bimodal_pred = bimodal_pred;
    last_gshare_pred = gshare_pred;
    last_pc = pc;
    
    // Use selector to choose which prediction to return
    uint32_t selector_idx = pc & selector_mask;
    uint8_t selector = selector_table[selector_idx];
    
    // Select based on selector value:
    // 0,1: Use bimodal (P1)
    // 2,3: Use gshare (P2)
    if (selector <= WEAK_BIMODAL) {
        return bimodal_pred;  // Use bimodal prediction
    } else {
        return gshare_pred;   // Use gshare prediction
    }
}

void tournament_predictor_train(uint32_t pc, uint8_t outcome) {
    if (!initialized) return;
    
    // Use stored predictions from predict phase
    if (pc != last_pc) {
        // Mismatch - this shouldn't happen in normal operation
        // Train components but don't update selector
        twobit_predictor_train(pc, outcome);
        gshare_predictor_train(pc, outcome);
        return;
    }
    
    // Train both component predictors (they need to learn regardless)
    twobit_predictor_train(pc, outcome);
    gshare_predictor_train(pc, outcome);
    
    // Update the selector based on which predictor was more accurate
    uint32_t selector_idx = pc & selector_mask;
    uint8_t selector = selector_table[selector_idx];
    
    // Use the stored predictions to evaluate accuracy
    bool bimodal_correct = (last_bimodal_pred == outcome);
    bool gshare_correct = (last_gshare_pred == outcome);
    
    // Update selector based on relative accuracy
    if (bimodal_correct && !gshare_correct) {
        // Bimodal was right, gshare was wrong -> favor bimodal
        if (selector > STRONG_BIMODAL) {
            selector_table[selector_idx] = selector - 1;
        }
    } else if (gshare_correct && !bimodal_correct) {
        // Gshare was right, bimodal was wrong -> favor gshare  
        if (selector < STRONG_GSHARE) {
            selector_table[selector_idx] = selector + 1;
        }
    }
    // If both correct or both wrong, don't update selector
}

void tournament_predictor_cleanup() {
    if (selector_table) {
        free(selector_table);
        selector_table = NULL;
    }
    
    // Clean up component predictors
    twobit_predictor_cleanup();
    gshare_predictor_cleanup();
    
    initialized = false;
}
