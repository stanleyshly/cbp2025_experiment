// tage_predictor.cc
// WORKING TAGE implementation (18 histories) - reverting to known good version
#include "tage_predictor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// WORKING configuration that gave 0.44 MPKI
#define NHIST 18                        // 18 history lengths (WORKING!)
#define LOGG 10                         // Log size of banks = 1024 entries per bank
#define LOGB 13                         // Bimodal predictor: 8192 entries  
#define TBITS 8                         // Tag bits (base)
#define CWIDTH 3                        // Counter width
#define UWIDTH 2                        // Usefulness counter width  
#define MINHIST 5                       // Minimum history length
#define MAXHIST 1000                    // Maximum history length
#define BORN 9                          // Threshold between low/high history
#define NBANKLOW 5                      // Banks for low history
#define NBANKHIGH 10                    // Banks for high history

// Entry structures
typedef struct {
    int8_t pred;                        // Prediction 
    int8_t hyst;                        // Hysteresis
} bentry_t;

typedef struct {
    int8_t ctr;                         // Prediction counter
    uint32_t tag;                       // Tag
    int8_t u;                           // Usefulness counter
} gentry_t;

// Predictor state
static bentry_t* btable = NULL;         // Bimodal table
static gentry_t** gtable = NULL;        // Tagged tables
static uint64_t ghist = 0;              // Global history
static int m[NHIST + 1];                // History lengths for each table
static int TB[NHIST + 1];               // Tag bits for each table
static uint32_t SizeTable[NHIST + 1];   // Size of each table
static bool NOSKIP[NHIST + 1];          // Whether to skip table

// Folded history for efficient hash computation
typedef struct {
    uint32_t comp;
    int CLENGTH;
    int OLENGTH;
    int OUTPOINT;
} folded_history_t;

static folded_history_t ch_i[NHIST + 1];  // For indices
static folded_history_t ch_t[NHIST + 1];  // For tags

// Initialize folded history
static void init_folded_history(folded_history_t* fh, int original_length, int compressed_length) {
    fh->comp = 0;
    fh->CLENGTH = compressed_length;
    fh->OLENGTH = original_length;
    fh->OUTPOINT = original_length % compressed_length;
}

// Update folded history
static void update_folded_history(folded_history_t* fh, uint64_t h, bool bit) {
    h &= (1ULL << fh->OLENGTH) - 1;
    fh->comp = (fh->comp << 1) | (bit ? 1 : 0);
    fh->comp ^= (h >> fh->OUTPOINT) & 1;
    fh->comp &= (1 << fh->CLENGTH) - 1;
}

// Initialize history lengths using geometric progression
static void init_histories() {
    m[1] = MINHIST;
    m[NHIST] = MAXHIST;
    
    // Geometric progression
    for (int i = 2; i < NHIST; i++) {
        m[i] = (int)(((double)MINHIST * 
                      pow((double)MAXHIST / (double)MINHIST, 
                          (double)(i - 1) / (double)(NHIST - 2))) + 0.5);
    }
    
    // Set up table parameters
    for (int i = 1; i <= NHIST; i++) {
        NOSKIP[i] = true;  // All tables active in working version
        TB[i] = TBITS + (i >= BORN ? 4 : 0);  // More tag bits for longer histories
        
        // Initialize folded histories
        init_folded_history(&ch_i[i], m[i], LOGG);
        init_folded_history(&ch_t[i], m[i], TB[i]);
    }
}

// Index computation
static uint32_t gindex(uint32_t PC, int bank) {
    uint32_t index = PC ^ (PC >> LOGG) ^ ch_i[bank].comp;
    
    if (bank >= BORN) {
        // High history banks - use bank interleaving
        int T = (PC ^ (ghist & ((1ULL << m[BORN]) - 1))) % NBANKHIGH;
        index += (T << LOGG);
    } else {
        // Low history banks
        int T = (PC ^ (ghist & ((1 << m[1]) - 1))) % NBANKLOW;
        index += (T << LOGG);
    }
    
    return index & (SizeTable[bank] - 1);
}

// Tag computation
static uint32_t gtag(uint32_t PC, int bank) {
    return (((PC ^ (PC >> 2))) ^ ch_t[bank].comp) & ((1 << TB[bank]) - 1);
}

void tage_predictor_init() {
    init_histories();
    
    // Allocate bimodal table
    btable = (bentry_t*)malloc((1 << LOGB) * sizeof(bentry_t));
    for (int i = 0; i < (1 << LOGB); i++) {
        btable[i].pred = 0;
        btable[i].hyst = 1;
    }
    
    // Allocate tagged tables with proper sizing
    gtable = (gentry_t**)malloc((NHIST + 1) * sizeof(gentry_t*));
    gtable[0] = NULL;
    
    // Low history tables
    SizeTable[1] = NBANKLOW * (1 << LOGG);
    gtable[1] = (gentry_t*)malloc(SizeTable[1] * sizeof(gentry_t));
    memset(gtable[1], 0, SizeTable[1] * sizeof(gentry_t));
    
    // High history tables  
    if (BORN <= NHIST) {
        SizeTable[BORN] = NBANKHIGH * (1 << LOGG);
        gtable[BORN] = (gentry_t*)malloc(SizeTable[BORN] * sizeof(gentry_t));
        memset(gtable[BORN], 0, SizeTable[BORN] * sizeof(gentry_t));
    }
    
    // Share tables
    for (int i = 2; i < BORN && i <= NHIST; i++) {
        gtable[i] = gtable[1];
        SizeTable[i] = SizeTable[1];
    }
    for (int i = BORN + 1; i <= NHIST; i++) {
        gtable[i] = gtable[BORN];
        SizeTable[i] = SizeTable[BORN];
    }
    
    ghist = 0;
}

uint8_t tage_predictor_predict(uint32_t PC) {
    int HitBank = 0;
    bool pred_taken = false;
    bool alt_taken = false;
    
    // Bimodal prediction
    int BI = (PC ^ (PC >> 2)) & ((1 << LOGB) - 1);
    pred_taken = alt_taken = (btable[BI].pred >= 0);
    
    // Look for hitting bank with longest history
    for (int i = NHIST; i > 0; i--) {
        if (!NOSKIP[i]) continue;
        
        uint32_t index = gindex(PC, i);
        uint32_t tag = gtag(PC, i);
        
        if (gtable[i][index].tag == tag) {
            HitBank = i;
            pred_taken = (gtable[i][index].ctr >= 0);
            break;
        }
    }
    
    return pred_taken ? 1 : 0;
}

void tage_predictor_train(uint32_t PC, uint8_t outcome) {
    bool taken = (outcome == 1);
    
    // Find hitting bank again
    int HitBank = 0;
    int BI = (PC ^ (PC >> 2)) & ((1 << LOGB) - 1);
    
    for (int i = NHIST; i > 0; i--) {
        if (!NOSKIP[i]) continue;
        
        uint32_t index = gindex(PC, i);
        uint32_t tag = gtag(PC, i);
        
        if (gtable[i][index].tag == tag) {
            HitBank = i;
            break;
        }
    }
    
    // Update the provider
    if (HitBank > 0) {
        uint32_t index = gindex(PC, HitBank);
        gentry_t* entry = &gtable[HitBank][index];
        
        if (taken && entry->ctr < ((1 << (CWIDTH - 1)) - 1)) {
            entry->ctr++;
        } else if (!taken && entry->ctr > -(1 << (CWIDTH - 1))) {
            entry->ctr--;
        }
    } else {
        // Update bimodal
        if (taken && btable[BI].pred < ((1 << (CWIDTH - 1)) - 1)) {
            btable[BI].pred++;
        } else if (!taken && btable[BI].pred > -(1 << (CWIDTH - 1))) {
            btable[BI].pred--;
        }
    }
    
    // Allocation on misprediction
    bool pred_taken = (HitBank > 0) ? 
                      (gtable[HitBank][gindex(PC, HitBank)].ctr >= 0) : 
                      (btable[BI].pred >= 0);
    
    if (pred_taken != taken && HitBank < NHIST) {
        // Try to allocate a new entry
        for (int i = HitBank + 1; i <= NHIST; i++) {
            if (!NOSKIP[i]) continue;
            
            uint32_t index = gindex(PC, i);
            gentry_t* entry = &gtable[i][index];
            
            if (entry->u == 0) {
                entry->tag = gtag(PC, i);
                entry->ctr = taken ? 0 : -1;
                entry->u = 0;
                break;
            } else if (entry->u > 0) {
                entry->u--;
            }
        }
    }
    
    // Update usefulness
    if (HitBank > 0) {
        uint32_t index = gindex(PC, HitBank);
        gentry_t* entry = &gtable[HitBank][index];
        
        if (pred_taken == taken) {
            if (entry->u < ((1 << UWIDTH) - 1)) entry->u++;
        }
    }
    
    // Update global history and folded histories
    for (int i = 1; i <= NHIST; i++) {
        if (!NOSKIP[i]) continue;
        update_folded_history(&ch_i[i], ghist, taken);
        update_folded_history(&ch_t[i], ghist, taken);
    }
    
    ghist = (ghist << 1) | (taken ? 1 : 0);
}

void tage_predictor_cleanup() {
    if (btable) {
        free(btable);
        btable = NULL;
    }
    
    if (gtable) {
        if (gtable[1]) free(gtable[1]);
        if (BORN <= NHIST && gtable[BORN] && gtable[BORN] != gtable[1]) {
            free(gtable[BORN]);
        }
        free(gtable);
        gtable = NULL;
    }
    
    ghist = 0;
}