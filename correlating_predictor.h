// correlating_predictor.h
// Header for n-bit correlating branch predictor
#ifndef CORRELATING_PREDICTOR_H
#define CORRELATING_PREDICTOR_H
#include <stdint.h>
void correlating_predictor_init(int table_bits, int history_bits, int n_bits);
uint8_t correlating_predictor_predict(uint32_t pc);
void correlating_predictor_train(uint32_t pc, uint8_t outcome);
void correlating_predictor_cleanup();
#endif
