#include <stdio.h>
#include "geder_truth_gate.h"

static GEDER_GateVerdict gate_always_continue(
    const GEDER_GateInput *input,
    void *gate_user_data
)
{
    long weight;

    (void)input;
    weight = *(const long *)gate_user_data;
    return GEDER_VerdictContinue(
        GEDER_TRUE,
        GEDER_FixedFromInt(weight),
        (GEDER_TruthMask)(1UL << input->gate_index),
        4000L + (long)input->gate_index
    );
}

int main(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;
    long weights[3];

    weights[0] = 1L;
    weights[1] = 2L;
    weights[2] = 3L;

    GEDER_PipelineInit(&pipeline);
    GEDER_PipelineSetEndPolicy(
        &pipeline,
        GEDER_END_ACCEPT_IF_MIN_TRUTHS_AND_SCORE,
        3u,
        GEDER_FixedFromInt(6L)
    );

    GEDER_PipelineAddGate(&pipeline, gate_always_continue, &weights[0], 0);
    GEDER_PipelineAddGate(&pipeline, gate_always_continue, &weights[1], 0);
    GEDER_PipelineAddGate(&pipeline, gate_always_continue, &weights[2], 0);

    GEDER_RunBegin(&run, &pipeline, 0, 0);

    while (!GEDER_RunIsFinished(&run))
    {
        GEDER_RunStep(&run);
        printf(
            "step: result=%s truths=%u score=%ld next=%u\n",
            GEDER_FinalResultName(run.final_result),
            run.accumulator.truths_accumulated,
            GEDER_FixedToIntFloor(run.accumulator.truth_score),
            run.next_gate_index
        );
    }

    return run.final_result == GEDER_FINAL_ACCEPTED ? 0 : 1;
}
