// twobit_predictor.h
// Header for 2-bit saturating counter branch predictor
#ifndef TWOBIT_PREDICTOR_H
#define TWOBIT_PREDICTOR_H
#include <stdint.h>
void twobit_predictor_init(int table_bits);
uint8_t twobit_predictor_predict(uint32_t pc);
void twobit_predictor_train(uint32_t pc, uint8_t outcome);
void twobit_predictor_cleanup();
#endif
