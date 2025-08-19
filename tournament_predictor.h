// tournament_predictor.h
// Tournament branch predictor that selects between bimodal and gshare predictors
#ifndef TOURNAMENT_PREDICTOR_H
#define TOURNAMENT_PREDICTOR_H

#include <stdint.h>

// Tournament predictor function declarations
void tournament_predictor_init(int selector_bits, int bimodal_bits, int gshare_table_bits, int gshare_history_bits);
uint8_t tournament_predictor_predict(uint32_t pc);
void tournament_predictor_train(uint32_t pc, uint8_t outcome);
void tournament_predictor_cleanup();

#endif // TOURNAMENT_PREDICTOR_H
