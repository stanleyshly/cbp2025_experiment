// perceptron_predictor.h
// Header for perceptron-based branch predictor
#ifndef PERCEPTRON_PREDICTOR_H
#define PERCEPTRON_PREDICTOR_H
#include <stdint.h>

void perceptron_predictor_init(int table_bits, int history_length, int weight_bits, int threshold);
uint8_t perceptron_predictor_predict(uint32_t pc);
void perceptron_predictor_train(uint32_t pc, uint8_t outcome);
void perceptron_predictor_update_history(uint8_t outcome);
void perceptron_predictor_cleanup();

#endif
