    
    // tage_predictor.h
// Clean TAGE (TAgged GEometric) branch predictor implementation
#ifndef TAGE_PREDICTOR_H
#define TAGE_PREDICTOR_H
#include <stdint.h>

void tage_predictor_init();
uint8_t tage_predictor_predict(uint32_t pc);
void tage_predictor_train(uint32_t pc, uint8_t outcome);
void tage_predictor_cleanup();

#endif
