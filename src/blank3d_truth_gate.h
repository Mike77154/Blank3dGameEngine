#ifndef BLANK3D_TRUTH_GATE_H
#define BLANK3D_TRUTH_GATE_H

#include "geder_truth_gate.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Blank3DTruthProfileTag {
    B3D_TRUTH_PROFILE_RETRO = 0,
    B3D_TRUTH_PROFILE_ACTIVE,
    B3D_TRUTH_PROFILE_PROXIMITY,
    B3D_TRUTH_PROFILE_STRICT
} Blank3DTruthProfile;

typedef struct Blank3DTruthFactsTag {
    int entity_alive;
    int target_alive;
    int socketer_has_target;
    int within_range;
    int line_of_sight;
    int eyes_visible;
    int eyes_partial;
    int enlightener_interpreted;
    int target_heard;
    int target_remembered;
} Blank3DTruthFacts;

typedef struct Blank3DTruthGateTag {
    GEDER_Pipeline pipeline;
    GEDER_Accumulator accumulator;
    GEDER_FinalResult last_result;
    Blank3DTruthProfile profile;
    long terminal_reason;
    unsigned long evaluations;
} Blank3DTruthGate;

void blank3d_truth_gate_init(Blank3DTruthGate *gate,
                             Blank3DTruthProfile profile);
int blank3d_truth_gate_set_profile(Blank3DTruthGate *gate,
                                   Blank3DTruthProfile profile);
int blank3d_truth_gate_set_profile_name(Blank3DTruthGate *gate,
                                        const char *profile_name);
Blank3DTruthProfile blank3d_truth_gate_profile_from_name(
    const char *profile_name);
const char *blank3d_truth_gate_profile_name(Blank3DTruthProfile profile);

int blank3d_truth_gate_evaluate(Blank3DTruthGate *gate,
                                const Blank3DTruthFacts *facts);
int blank3d_truth_gate_is_accepted(const Blank3DTruthGate *gate);
unsigned int blank3d_truth_gate_truth_count(const Blank3DTruthGate *gate);
long blank3d_truth_gate_score_floor(const Blank3DTruthGate *gate);
long blank3d_truth_gate_reason(const Blank3DTruthGate *gate);
int blank3d_truth_gate_has_mask(const Blank3DTruthGate *gate,
                                unsigned long truth_mask);

#ifdef __cplusplus
}
#endif

#endif /* BLANK3D_TRUTH_GATE_H */
