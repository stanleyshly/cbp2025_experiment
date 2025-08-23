// perceptron_predictor.cc
// Perceptron-based branch predictor implementation
#include "perceptron_predictor.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

// Perceptron predictor parameters
static int num_perceptrons = 0;          // Number of perceptrons in table (2^table_bits)
static int history_len = 0;              // Length of global history register
static int weight_bits = 0;              // Number of bits for each weight
static int threshold = 0;                // Training threshold
static uint32_t table_mask = 0;          // Mask for indexing into perceptron table

// Data structures
static int32_t** perceptron_table = NULL; // Table of perceptrons (each is array of weights)
static int32_t* global_history = NULL;    // Global history register (bipolar: -1/+1)
static int history_index = 0;            // Current position in circular history buffer
static int32_t max_weight = 0;           // Maximum weight value (for saturation)
static int32_t min_weight = 0;           // Minimum weight value (for saturation)

void perceptron_predictor_init(int table_bits, int history_length, int weight_bits_param, int threshold_param) {
    // Set parameters
    num_perceptrons = 1 << table_bits;
    history_len = history_length;
    weight_bits = weight_bits_param;
    threshold = threshold_param;
    table_mask = num_perceptrons - 1;
    
    // Calculate weight saturation bounds
    max_weight = (1 << (weight_bits - 1)) - 1;    // e.g., for 8 bits: 127
    min_weight = -(1 << (weight_bits - 1));       // e.g., for 8 bits: -128
    
    // Allocate perceptron table
    perceptron_table = (int32_t**)malloc(num_perceptrons * sizeof(int32_t*));
    for (int i = 0; i < num_perceptrons; i++) {
        // Each perceptron has history_len + 1 weights (including bias weight w0)
        perceptron_table[i] = (int32_t*)malloc((history_len + 1) * sizeof(int32_t));
        // Initialize all weights to 0
        memset(perceptron_table[i], 0, (history_len + 1) * sizeof(int32_t));
    }
    
    // Allocate and initialize global history register
    global_history = (int32_t*)malloc(history_len * sizeof(int32_t));
    // Initialize history to not taken (-1)
    for (int i = 0; i < history_len; i++) {
        global_history[i] = -1;
    }
    history_index = 0;
}

uint8_t perceptron_predictor_predict(uint32_t pc) {
    // Hash PC to select perceptron
    uint32_t perceptron_idx = pc & table_mask;
    int32_t* weights = perceptron_table[perceptron_idx];
    
    // Compute dot product: y = w0 + sum(xi * wi) for i=1 to history_len
    int32_t y = weights[0]; // Start with bias weight w0
    
    for (int i = 0; i < history_len; i++) {
        y += global_history[i] * weights[i + 1];
    }
    
    // Predict taken if y >= 0, not taken if y < 0
    return (y >= 0) ? 1 : 0;
}

void perceptron_predictor_train(uint32_t pc, uint8_t outcome) {
    // Hash PC to select perceptron
    uint32_t perceptron_idx = pc & table_mask;
    int32_t* weights = perceptron_table[perceptron_idx];
    
    // Compute current output
    int32_t y = weights[0]; // Start with bias weight w0
    for (int i = 0; i < history_len; i++) {
        y += global_history[i] * weights[i + 1];
    }
    
    // Convert outcome to bipolar: 0 -> -1, 1 -> +1
    int32_t target = (outcome == 1) ? 1 : -1;
    
    // Train if prediction was wrong OR if |y| <= threshold
    bool wrong_prediction = ((y >= 0) ? 1 : 0) != outcome;
    bool within_threshold = (y >= 0) ? (y <= threshold) : (-y <= threshold);
    
    if (wrong_prediction || within_threshold) {
        // Update bias weight w0 (input is always 1)
        weights[0] += target;
        // Saturate weight
        if (weights[0] > max_weight) weights[0] = max_weight;
        if (weights[0] < min_weight) weights[0] = min_weight;
        
        // Update history weights w1 to wn
        for (int i = 0; i < history_len; i++) {
            weights[i + 1] += target * global_history[i];
            // Saturate weight
            if (weights[i + 1] > max_weight) weights[i + 1] = max_weight;
            if (weights[i + 1] < min_weight) weights[i + 1] = min_weight;
        }
    }
}

void perceptron_predictor_update_history(uint8_t outcome) {
    // Convert outcome to bipolar and update global history
    int32_t bipolar_outcome = (outcome == 1) ? 1 : -1;
    
    // Update circular history buffer
    global_history[history_index] = bipolar_outcome;
    history_index = (history_index + 1) % history_len;
}

void perceptron_predictor_cleanup() {
    if (perceptron_table) {
        for (int i = 0; i < num_perceptrons; i++) {
            if (perceptron_table[i]) {
                free(perceptron_table[i]);
            }
        }
        free(perceptron_table);
        perceptron_table = NULL;
    }
    
    if (global_history) {
        free(global_history);
        global_history = NULL;
    }
}
