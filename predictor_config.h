// predictor_config.h
// Dynamic configuration system for branch predictors
#ifndef PREDICTOR_CONFIG_H
#define PREDICTOR_CONFIG_H

#include <cstdlib>
#include <string>

// Configuration structure for each predictor type
struct PredictorConfig {
    // Onebit predictor
    int onebit_table_bits = 17;
    
    // Twobit predictor  
    int twobit_table_bits = 17;
    
    // Gshare predictor
    int gshare_table_bits = 17;
    int gshare_history_bits = 4;
    
    // Correlating predictor
    int correlating_pc_bits = 14;
    int correlating_history_bits = 4;
    int correlating_counter_bits = 2;
    
    // Local predictor
    int local_lht_bits = 14;
    int local_history_bits = 6;
    int local_pht_bits = 12;
    
    // Tournament predictor
    int tournament_selector_bits = 14;
    int tournament_bimodal_bits = 14;
    int tournament_gshare_table_bits = 14;
    int tournament_gshare_history_bits = 6;
};

// Global configuration instance
extern PredictorConfig g_predictor_config;

// Function to load configuration from environment variables
inline void load_config_from_env() {
    // Onebit
    if (const char* val = std::getenv("ONEBIT_TABLE_BITS")) {
        g_predictor_config.onebit_table_bits = std::atoi(val);
    }
    
    // Twobit
    if (const char* val = std::getenv("TWOBIT_TABLE_BITS")) {
        g_predictor_config.twobit_table_bits = std::atoi(val);
    }
    
    // Gshare
    if (const char* val = std::getenv("GSHARE_TABLE_BITS")) {
        g_predictor_config.gshare_table_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("GSHARE_HISTORY_BITS")) {
        g_predictor_config.gshare_history_bits = std::atoi(val);
    }
    
    // Correlating
    if (const char* val = std::getenv("CORRELATING_PC_BITS")) {
        g_predictor_config.correlating_pc_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("CORRELATING_HISTORY_BITS")) {
        g_predictor_config.correlating_history_bits = std::atoi(val);
    }
    
    // Local
    if (const char* val = std::getenv("LOCAL_LHT_BITS")) {
        g_predictor_config.local_lht_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("LOCAL_HISTORY_BITS")) {
        g_predictor_config.local_history_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("LOCAL_PHT_BITS")) {
        g_predictor_config.local_pht_bits = std::atoi(val);
    }
    
    // Tournament
    if (const char* val = std::getenv("TOURNAMENT_SELECTOR_BITS")) {
        g_predictor_config.tournament_selector_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("TOURNAMENT_BIMODAL_BITS")) {
        g_predictor_config.tournament_bimodal_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("TOURNAMENT_GSHARE_TABLE_BITS")) {
        g_predictor_config.tournament_gshare_table_bits = std::atoi(val);
    }
    if (const char* val = std::getenv("TOURNAMENT_GSHARE_HISTORY_BITS")) {
        g_predictor_config.tournament_gshare_history_bits = std::atoi(val);
    }
}

// Function to print current configuration
inline void print_config() {
    printf("=== Predictor Configuration ===\n");
    printf("Onebit: table_bits=%d\n", g_predictor_config.onebit_table_bits);
    printf("Twobit: table_bits=%d\n", g_predictor_config.twobit_table_bits);
    printf("Gshare: table_bits=%d, history_bits=%d\n", 
           g_predictor_config.gshare_table_bits, g_predictor_config.gshare_history_bits);
    printf("Correlating: pc_bits=%d, history_bits=%d, counter_bits=%d\n",
           g_predictor_config.correlating_pc_bits, g_predictor_config.correlating_history_bits, 
           g_predictor_config.correlating_counter_bits);
    printf("Local: lht_bits=%d, history_bits=%d, pht_bits=%d\n",
           g_predictor_config.local_lht_bits, g_predictor_config.local_history_bits,
           g_predictor_config.local_pht_bits);
    printf("Tournament: selector_bits=%d, bimodal_bits=%d, gshare_table_bits=%d, gshare_history_bits=%d\n",
           g_predictor_config.tournament_selector_bits, g_predictor_config.tournament_bimodal_bits,
           g_predictor_config.tournament_gshare_table_bits, g_predictor_config.tournament_gshare_history_bits);
    printf("===============================\n");
}

#endif // PREDICTOR_CONFIG_H
