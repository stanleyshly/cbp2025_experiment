/*
  Copyright (C) ARM Limited 2008-2025  All rights reserved.                                                                                                                                                                                                                        

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// This file provides a sample predictor integration based on the interface provided.

// This file provides a sample predictor integration based on the interface provided.

#include "cond_branch_predictor_interface.h"
#include "lib/predictor_type.h"
#include "lib/sim_common_structs.h"
#include "cbp2016_tage_sc_l.h"
#include "my_cond_branch_predictor.h"

#include "onebit_predictor.h"
#include "twobit_predictor.h"
#include "correlating_predictor.h"
#include "local_predictor.h"
#include "gshare_predictor.h"
#include "tournament_predictor.h"
#include "predictor_config.h"

void select_predictor(PredictorType pt) {
    selected_predictor = pt;
}
#include <cassert>

//
// beginCondDirPredictor()
// 
// This function is called by the simulator before the start of simulation.
// It can be used for arbitrary initialization steps for the contestant's code.
//
void beginCondDirPredictor()
{
    // Load dynamic configuration from environment variables
    load_config_from_env();
    
    // Print configuration if debug mode
    if (std::getenv("PRINT_PREDICTOR_CONFIG")) {
        print_config();
    }
    
    // setup predictors
    cbp2016_tage_sc_l.setup();
    cond_predictor_impl.setup();
    if (get_selected_predictor() == PredictorType::PRED_ONEBIT) {
        onebit_predictor_init(g_predictor_config.onebit_table_bits);
    } else if (get_selected_predictor() == PredictorType::PRED_TWOBIT) {
        twobit_predictor_init(g_predictor_config.twobit_table_bits);
    } else if (get_selected_predictor() == PredictorType::PRED_CORRELATING) {
        correlating_predictor_init(g_predictor_config.correlating_pc_bits, 
                                   g_predictor_config.correlating_history_bits, 
                                   g_predictor_config.correlating_counter_bits);
    } else if (get_selected_predictor() == PredictorType::PRED_LOCAL) {
        local_predictor_init(g_predictor_config.local_lht_bits, 
                            g_predictor_config.local_history_bits, 
                            g_predictor_config.local_pht_bits);
    } else if (get_selected_predictor() == PredictorType::PRED_GSHARE) {
        gshare_predictor_init(g_predictor_config.gshare_table_bits, 
                             g_predictor_config.gshare_history_bits);
    } else if (get_selected_predictor() == PredictorType::PRED_TOURNAMENT) {
        tournament_predictor_init(g_predictor_config.tournament_selector_bits, 
                                 g_predictor_config.tournament_bimodal_bits,
                                 g_predictor_config.tournament_gshare_table_bits, 
                                 g_predictor_config.tournament_gshare_history_bits);
    }
}

//
// notify_instr_fetch(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t fetch_cycle)
// 
// This function is called when any instructions(not just branches) gets fetched.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction and fetch_cycle are also provided as inputs
//
void notify_instr_fetch(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t fetch_cycle)
{
}

//
// get_cond_dir_prediction(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t pred_cycle)
// 
// This function is called by the simulator for predicting conditional branches.
// input values are unique identifying ids(seq_no, piece) and PC of the branch.
// return value is the predicted direction. 
//
bool get_cond_dir_prediction(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t pred_cycle)
{
    if (get_selected_predictor() == PredictorType::PRED_ONEBIT) {
        return onebit_predictor_predict((uint32_t)pc);
    } else if (get_selected_predictor() == PredictorType::PRED_TWOBIT) {
        return twobit_predictor_predict((uint32_t)pc);
    } else if (get_selected_predictor() == PredictorType::PRED_CORRELATING) {
        return correlating_predictor_predict((uint32_t)pc);
    } else if (get_selected_predictor() == PredictorType::PRED_LOCAL) {
        return local_predictor_predict((uint32_t)pc);
    } else if (get_selected_predictor() == PredictorType::PRED_GSHARE) {
        return gshare_predictor_predict((uint32_t)pc);
    } else if (get_selected_predictor() == PredictorType::PRED_TOURNAMENT) {
        return tournament_predictor_predict((uint32_t)pc);
    }
    const bool tage_sc_l_pred =  cbp2016_tage_sc_l.predict(seq_no, piece, pc);
    const bool my_prediction = cond_predictor_impl.predict(seq_no, piece, pc, tage_sc_l_pred);
    return my_prediction;
}

//
// spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, InstClass inst_class, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
// 
// This function is called by the simulator for updating the history vectors and any state that needs to be updated speculatively.
// The function is called for all the branches (not just conditional branches). To faciliate accurate history updates, spec_update is called right
// after a prediction is made.
// input values are unique identifying ids(seq_no, piece), PC of the instruction, instruction class, predicted/resolve direction and the next_pc 
//
void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, InstClass inst_class, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    assert(is_br(inst_class));
    int br_type = 0;
    switch(inst_class)
    {
        case InstClass::condBranchInstClass:
            br_type = 1;
            break;
        case InstClass::uncondDirectBranchInstClass:
            br_type = 0; 
            break;
        case InstClass::uncondIndirectBranchInstClass:
            br_type = 2;
            break;
        case InstClass::callDirectInstClass:
            br_type = 0;
            break;
        case InstClass::callIndirectInstClass:
            br_type = 2; 
            break;
        case InstClass::ReturnInstClass:
            br_type = 2;
            break;
        default:
            assert(false);
    }

    if(inst_class == InstClass::condBranchInstClass)
    {
        if (get_selected_predictor() == PredictorType::PRED_ONEBIT) {
            onebit_predictor_train((uint32_t)pc, resolve_dir);
        } else if (get_selected_predictor() == PredictorType::PRED_TWOBIT) {
            twobit_predictor_train((uint32_t)pc, resolve_dir);
        } else if (get_selected_predictor() == PredictorType::PRED_CORRELATING) {
            correlating_predictor_train((uint32_t)pc, resolve_dir);
        } else if (get_selected_predictor() == PredictorType::PRED_LOCAL) {
            local_predictor_train((uint32_t)pc, resolve_dir);
        } else if (get_selected_predictor() == PredictorType::PRED_GSHARE) {
            gshare_predictor_train((uint32_t)pc, resolve_dir);
        } else {
            cbp2016_tage_sc_l.history_update(seq_no, piece, pc, br_type, pred_dir, resolve_dir, next_pc);
            cond_predictor_impl.history_update(seq_no, piece, pc, resolve_dir, next_pc);
        }
    }
    else
    {
        cbp2016_tage_sc_l.TrackOtherInst(pc, br_type, pred_dir, resolve_dir, next_pc);
    }

}

//
// notify_instr_decode(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t decode_cycle)
// 
// This function is called when any instructions(not just branches) gets decoded.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, decode info and cycle are also provided as inputs
//
// For the sample predictor implementation, we do not leverage decode information
void notify_instr_decode(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t decode_cycle)
{
}

//
// notify_agen_complete(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t mem_va, const uint64_t mem_sz, const uint64_t agen_cycle)
// 
// This function is called when any load/store instructions complete agen.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, decode info, mem_va and mem_sz and agen_cycle are also provided as inputs
//
void notify_agen_complete(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t mem_va, const uint64_t mem_sz, const uint64_t agen_cycle)
{
}

//
// notify_instr_execute_resolve(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t execute_cycle)
// 
// This function is called when any instructions(not just branches) gets executed.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, execute info and cycle are also provided as inputs
//
// For conditional branches, we use this information to update the predictor.
// At the moment, we do not consider updating any other structure, but the contestants are allowed to  update any other predictor state.
void notify_instr_execute_resolve(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t execute_cycle)
{
    const bool is_branch = is_br(_exec_info.dec_info.insn_class);
    if(is_branch)
    {
        if (is_cond_br(_exec_info.dec_info.insn_class))
        {
            const bool _resolve_dir = _exec_info.taken.value();
            const uint64_t _next_pc = _exec_info.next_pc;
            if (get_selected_predictor() == PredictorType::PRED_ONEBIT) {
                onebit_predictor_train((uint32_t)pc, _resolve_dir);
            } else if (get_selected_predictor() == PredictorType::PRED_TWOBIT) {
                twobit_predictor_train((uint32_t)pc, _resolve_dir);
            } else if (get_selected_predictor() == PredictorType::PRED_CORRELATING) {
                correlating_predictor_train((uint32_t)pc, _resolve_dir);
            } else if (get_selected_predictor() == PredictorType::PRED_LOCAL) {
                local_predictor_train((uint32_t)pc, _resolve_dir);
            } else if (get_selected_predictor() == PredictorType::PRED_GSHARE) {
                gshare_predictor_train((uint32_t)pc, _resolve_dir);
            } else if (get_selected_predictor() == PredictorType::PRED_TOURNAMENT) {
                tournament_predictor_train((uint32_t)pc, _resolve_dir);
            } else {
                cbp2016_tage_sc_l.update(seq_no, piece, pc, _resolve_dir, pred_dir, _next_pc);
                cond_predictor_impl.update(seq_no, piece, pc, _resolve_dir, pred_dir, _next_pc);
            }
        }
        else
        {
            assert(pred_dir);
        }
    }
}

//
// notify_instr_commit(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t commit_cycle)
// 
// This function is called when any instructions(not just branches) gets committed.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, execute info and cycle are also provided as inputs
//
// For the sample predictor implementation, we do not leverage commit information
void notify_instr_commit(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t commit_cycle)
{
}

//
// endCondDirPredictor()
//
// This function is called by the simulator at the end of simulation.
// It can be used by the contestant to print out other contestant-specific measurements.
//
void endCondDirPredictor ()
{
    if (get_selected_predictor() == PredictorType::PRED_ONEBIT) {
        onebit_predictor_cleanup();
    } else if (get_selected_predictor() == PredictorType::PRED_TWOBIT) {
        twobit_predictor_cleanup();
    } else if (get_selected_predictor() == PredictorType::PRED_CORRELATING) {
        correlating_predictor_cleanup();
    } else if (get_selected_predictor() == PredictorType::PRED_LOCAL) {
        local_predictor_cleanup();
    } else if (get_selected_predictor() == PredictorType::PRED_GSHARE) {
        gshare_predictor_cleanup();
    } else if (get_selected_predictor() == PredictorType::PRED_TOURNAMENT) {
        tournament_predictor_cleanup();
    } else {
        cbp2016_tage_sc_l.terminate();
        cond_predictor_impl.terminate();
    }
}
