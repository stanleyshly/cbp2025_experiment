// onebit_predictor.h
// Header for 1-bit branch predictor
#ifndef ONEBIT_PREDICTOR_H
#define ONEBIT_PREDICTOR_H
#include <stdint.h>
void onebit_predictor_init(int table_bits);
uint8_t onebit_predictor_predict(uint32_t pc);
void onebit_predictor_train(uint32_t pc, uint8_t outcome);
void onebit_predictor_cleanup();
#endif
