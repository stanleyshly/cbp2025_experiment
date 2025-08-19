#ifndef PREDICTOR_TYPE_H
#define PREDICTOR_TYPE_H

enum class PredictorType {
    PRED_TAGE_SC_L,
    PRED_SAMPLE,
    PRED_GSHARE,
    PRED_TOURNAMENT,
    PRED_TAGE,
    PRED_ONEBIT,
    PRED_TWOBIT,
    PRED_CORRELATING,
    PRED_LOCAL
};

// Static variable to hold the selected predictor
static PredictorType selected_predictor = PredictorType::PRED_TAGE_SC_L;

// Inline function to get the selected predictor
inline PredictorType get_selected_predictor() {
    return selected_predictor;
}

// Inline function to set the selected predictor
inline void set_selected_predictor(PredictorType pt) {
    selected_predictor = pt;
}

#endif // PREDICTOR_TYPE_H
