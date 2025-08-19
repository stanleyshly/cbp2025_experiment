// Only define PredictorType once
#ifndef COND_BRANCH_PREDICTOR_INTERFACE_H
#define COND_BRANCH_PREDICTOR_INTERFACE_H

#include <stdint.h>
#include "lib/predictor_type.h"
#include "lib/sim_common_structs.h"
void select_predictor(PredictorType pt);
void beginCondDirPredictor();
bool get_cond_dir_prediction(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t pred_cycle);
void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, InstClass inst_class, bool resolve_dir, bool pred_dir, uint64_t next_pc);
void endCondDirPredictor();

#endif // COND_BRANCH_PREDICTOR_INTERFACE_H
