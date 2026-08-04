#include "blank3d_truth_gate.h"

#include <string.h>

#define B3D_TRUTH_REASON_RETRO_ACCEPT       100L
#define B3D_TRUTH_REASON_ENTITY_DEAD        201L
#define B3D_TRUTH_REASON_ENTITY_ALIVE       202L
#define B3D_TRUTH_REASON_TARGET_DOWN        301L
#define B3D_TRUTH_REASON_TARGET_ALIVE       302L
#define B3D_TRUTH_REASON_SOCKET_MISSING     401L
#define B3D_TRUTH_REASON_SOCKET_KNOWN       402L
#define B3D_TRUTH_REASON_OUT_OF_RANGE       501L
#define B3D_TRUTH_REASON_IN_RANGE           502L
#define B3D_TRUTH_REASON_OCCLUDED           601L
#define B3D_TRUTH_REASON_VISIBLE            602L
#define B3D_TRUTH_REASON_EYES_FAILED        701L
#define B3D_TRUTH_REASON_EYES_VISIBLE       702L
#define B3D_TRUTH_REASON_ENLIGHTENER_EMPTY  801L
#define B3D_TRUTH_REASON_ENLIGHTENER_TRUTH  802L
#define B3D_TRUTH_REASON_SOUND_HEARD        901L
#define B3D_TRUTH_REASON_MEMORY_PRESENT     902L

#define B3D_TRUTH_MASK_ENTITY       1UL
#define B3D_TRUTH_MASK_TARGET       2UL
#define B3D_TRUTH_MASK_SOCKETER     4UL
#define B3D_TRUTH_MASK_RANGE        8UL
#define B3D_TRUTH_MASK_LOS         16UL
#define B3D_TRUTH_MASK_EYES        32UL
#define B3D_TRUTH_MASK_ENLIGHTENER 64UL
#define B3D_TRUTH_MASK_HEARING    128UL
#define B3D_TRUTH_MASK_MEMORY     256UL

static const Blank3DTruthFacts *b3d_truth_facts(
    const GEDER_GateInput *input)
{
    if (!input) return (const Blank3DTruthFacts *)0;
    return (const Blank3DTruthFacts *)input->world;
}

static GEDER_GateVerdict b3d_truth_retro_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->socketer_has_target)
        return GEDER_VerdictReject(B3D_TRUTH_REASON_SOCKET_MISSING);
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
                                 B3D_TRUTH_MASK_SOCKETER,
                                 B3D_TRUTH_REASON_RETRO_ACCEPT);
}

static GEDER_GateVerdict b3d_truth_entity_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->entity_alive)
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_ENTITY_DEAD);
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
                                 B3D_TRUTH_MASK_ENTITY,
                                 B3D_TRUTH_REASON_ENTITY_ALIVE);
}

static GEDER_GateVerdict b3d_truth_target_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->target_alive)
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_TARGET_DOWN);
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
                                 B3D_TRUTH_MASK_TARGET,
                                 B3D_TRUTH_REASON_TARGET_ALIVE);
}

static GEDER_GateVerdict b3d_truth_range_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->within_range)
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_OUT_OF_RANGE);
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
                                 B3D_TRUTH_MASK_RANGE,
                                 B3D_TRUTH_REASON_IN_RANGE);
}

static GEDER_GateVerdict b3d_truth_los_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->line_of_sight)
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_OCCLUDED);
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
                                 B3D_TRUTH_MASK_LOS,
                                 B3D_TRUTH_REASON_VISIBLE);
}

static GEDER_GateVerdict b3d_truth_eyes_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || (!facts->eyes_visible && !facts->eyes_partial))
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_EYES_FAILED);
    return GEDER_VerdictContinue(GEDER_TRUE,
        facts->eyes_visible ? GEDER_FixedFromInt(2L)
                            : GEDER_FixedFromInt(1L),
        B3D_TRUTH_MASK_EYES, B3D_TRUTH_REASON_EYES_VISIBLE);
}

static GEDER_GateVerdict b3d_truth_enlightener_gate(
    const GEDER_GateInput *input, void *user_data)
{
    const Blank3DTruthFacts *facts;
    GEDER_TruthMask mask;
    (void)user_data;
    facts = b3d_truth_facts(input);
    if (!facts || !facts->enlightener_interpreted)
        return GEDER_VerdictContinue(GEDER_FALSE, 0L, 0UL,
                                     B3D_TRUTH_REASON_ENLIGHTENER_EMPTY);
    mask = B3D_TRUTH_MASK_ENLIGHTENER;
    if (facts->target_heard) mask |= B3D_TRUTH_MASK_HEARING;
    if (facts->target_remembered) mask |= B3D_TRUTH_MASK_MEMORY;
    return GEDER_VerdictContinue(GEDER_TRUE, GEDER_FixedFromInt(1L),
        mask, facts->target_remembered ? B3D_TRUTH_REASON_MEMORY_PRESENT
                                      : B3D_TRUTH_REASON_SOUND_HEARD);
}

static int b3d_truth_add(GEDER_Pipeline *pipeline,
                         GEDER_GateFunction function)
{
    return GEDER_PipelineAddGate(pipeline, function, 0, 0);
}

static int b3d_truth_build_pipeline(Blank3DTruthGate *gate,
                                    Blank3DTruthProfile profile)
{
    if (!gate) return 0;
    GEDER_PipelineInit(&gate->pipeline);
    gate->profile = profile;

    /* Socketer is the basal absolute truth. Missing it is the only sensory
       failure that rejects execution. Every higher layer is additive and may
       fail without erasing the basal target. */
    if (!b3d_truth_add(&gate->pipeline, b3d_truth_retro_gate)) return 0;
    if (profile != B3D_TRUTH_PROFILE_RETRO) {
        if (!b3d_truth_add(&gate->pipeline, b3d_truth_entity_gate)) return 0;
        if (!b3d_truth_add(&gate->pipeline, b3d_truth_target_gate)) return 0;
    }
    if (profile == B3D_TRUTH_PROFILE_PROXIMITY ||
        profile == B3D_TRUTH_PROFILE_STRICT) {
        if (!b3d_truth_add(&gate->pipeline, b3d_truth_range_gate)) return 0;
        if (!b3d_truth_add(&gate->pipeline, b3d_truth_eyes_gate)) return 0;
    }
    if (profile == B3D_TRUTH_PROFILE_STRICT) {
        if (!b3d_truth_add(&gate->pipeline, b3d_truth_los_gate)) return 0;
        if (!b3d_truth_add(&gate->pipeline,
                           b3d_truth_enlightener_gate)) return 0;
    }

    return GEDER_PipelineSetEndPolicy(&gate->pipeline,
        GEDER_END_ACCEPT_IF_ANY_TRUTH, 1U, GEDER_FixedFromInt(1L));
}

void blank3d_truth_gate_init(Blank3DTruthGate *gate,
                             Blank3DTruthProfile profile)
{
    if (!gate) return;
    memset(gate, 0, sizeof(*gate));
    gate->last_result = GEDER_FINAL_PENDING;
    if (!b3d_truth_build_pipeline(gate, profile))
        gate->last_result = GEDER_FINAL_ERROR;
}

int blank3d_truth_gate_set_profile(Blank3DTruthGate *gate,
                                   Blank3DTruthProfile profile)
{
    if (!gate) return 0;
    gate->last_result = GEDER_FINAL_PENDING;
    gate->terminal_reason = GEDER_REASON_NONE;
    memset(&gate->accumulator, 0, sizeof(gate->accumulator));
    return b3d_truth_build_pipeline(gate, profile);
}

Blank3DTruthProfile blank3d_truth_gate_profile_from_name(
    const char *profile_name)
{
    if (!profile_name) return B3D_TRUTH_PROFILE_RETRO;
    if (strcmp(profile_name, "active") == 0 ||
        strcmp(profile_name, "alive") == 0)
        return B3D_TRUTH_PROFILE_ACTIVE;
    if (strcmp(profile_name, "proximity") == 0 ||
        strcmp(profile_name, "distance") == 0 ||
        strcmp(profile_name, "filtered") == 0)
        return B3D_TRUTH_PROFILE_PROXIMITY;
    if (strcmp(profile_name, "strict") == 0 ||
        strcmp(profile_name, "modern") == 0 ||
        strcmp(profile_name, "los") == 0 ||
        strcmp(profile_name, "sensory") == 0)
        return B3D_TRUTH_PROFILE_STRICT;
    return B3D_TRUTH_PROFILE_RETRO;
}

int blank3d_truth_gate_set_profile_name(Blank3DTruthGate *gate,
                                        const char *profile_name)
{
    return blank3d_truth_gate_set_profile(
        gate, blank3d_truth_gate_profile_from_name(profile_name));
}

const char *blank3d_truth_gate_profile_name(Blank3DTruthProfile profile)
{
    if (profile == B3D_TRUTH_PROFILE_ACTIVE) return "active";
    if (profile == B3D_TRUTH_PROFILE_PROXIMITY) return "proximity";
    if (profile == B3D_TRUTH_PROFILE_STRICT) return "strict";
    return "retro";
}

int blank3d_truth_gate_evaluate(Blank3DTruthGate *gate,
                                const Blank3DTruthFacts *facts)
{
    if (!gate || !facts) return 0;
    gate->last_result = GEDER_PipelineEvaluate(
        &gate->pipeline, facts, 0, &gate->accumulator,
        &gate->terminal_reason);
    gate->evaluations += 1UL;
    return gate->last_result == GEDER_FINAL_ACCEPTED;
}

int blank3d_truth_gate_is_accepted(const Blank3DTruthGate *gate)
{
    return gate && gate->last_result == GEDER_FINAL_ACCEPTED;
}

unsigned int blank3d_truth_gate_truth_count(const Blank3DTruthGate *gate)
{
    return gate ? gate->accumulator.truths_accumulated : 0U;
}

long blank3d_truth_gate_score_floor(const Blank3DTruthGate *gate)
{
    return gate ? GEDER_FixedToIntFloor(gate->accumulator.truth_score) : 0L;
}

long blank3d_truth_gate_reason(const Blank3DTruthGate *gate)
{
    return gate ? gate->terminal_reason : GEDER_REASON_NONE;
}

int blank3d_truth_gate_has_mask(const Blank3DTruthGate *gate,
                                unsigned long truth_mask)
{
    if (!gate) return 0;
    return (gate->accumulator.truth_mask & truth_mask) == truth_mask;
}
