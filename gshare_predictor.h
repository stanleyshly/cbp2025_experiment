// gshare_predictor.h
// Header for gshare branch predictor
#ifndef GSHARE_PREDICTOR_H
#define GSHARE_PREDICTOR_H
#include <stdint.h>
void gshare_predictor_init(int table_bits, int history_bits);
uint8_t gshare_predictor_predict(uint32_t pc);
void gshare_predictor_train(uint32_t pc, uint8_t outcome);
void gshare_predictor_cleanup();
#endif
