// local_predictor.h
// Header for local (two-level) branch predictor
#ifndef LOCAL_PREDICTOR_H
#define LOCAL_PREDICTOR_H
#include <stdint.h>
void local_predictor_init(int lht_bits, int history_bits, int pht_bits);
uint8_t local_predictor_predict(uint32_t pc);
void local_predictor_train(uint32_t pc, uint8_t outcome);
void local_predictor_cleanup();
#endif
