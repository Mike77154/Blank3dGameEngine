#include <assert.h>
#include <stdio.h>
#include "geder_truth_gate.h"

static GEDER_GateVerdict gate_continue_one(
    const GEDER_GateInput *input,
    void *user_data
)
{
    (void)input;
    (void)user_data;
    return GEDER_VerdictContinue(
        GEDER_TRUE,
        GEDER_FixedFromInt(1L),
        1UL,
        11L
    );
}

static GEDER_GateVerdict gate_accept_two(
    const GEDER_GateInput *input,
    void *user_data
)
{
    (void)input;
    (void)user_data;
    return GEDER_VerdictAccept(
        GEDER_TRUE,
        GEDER_FixedFromInt(2L),
        2UL,
        22L
    );
}

static GEDER_GateVerdict gate_reject(
    const GEDER_GateInput *input,
    void *user_data
)
{
    (void)input;
    (void)user_data;
    return GEDER_VerdictReject(33L);
}


static void test_fixed_helpers(void)
{
    GEDER_Fixed positive;
    GEDER_Fixed negative;

    positive = GEDER_FixedFromInt(5L) + GEDER_FIXED_HALF;
    negative = GEDER_FixedFromInt(-5L) - GEDER_FIXED_HALF;

    assert(GEDER_FixedToIntFloor(positive) == 5L);
    assert(GEDER_FixedToIntRound(positive) == 6L);
    assert(GEDER_FixedToIntFloor(negative) == -6L);
    assert(GEDER_FixedToIntRound(negative) == -6L);
    assert(GEDER_FixedAddSaturating(LONG_MAX, 1L) == LONG_MAX);
    assert(GEDER_FixedAddSaturating(LONG_MIN, -1L) == LONG_MIN);
}

static void test_accept_chain(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Accumulator accumulator;
    GEDER_FinalResult result;
    long reason;

    GEDER_PipelineInit(&pipeline);
    assert(GEDER_PipelineAddGate(&pipeline, gate_continue_one, 0, 0));
    assert(GEDER_PipelineAddGate(&pipeline, gate_accept_two, 0, 0));

    result = GEDER_PipelineEvaluate(
        &pipeline,
        0,
        0,
        &accumulator,
        &reason
    );

    assert(result == GEDER_FINAL_ACCEPTED);
    assert(accumulator.gates_visited == 2u);
    assert(accumulator.truths_accumulated == 2u);
    assert(GEDER_FixedToIntFloor(accumulator.truth_score) == 3L);
    assert(accumulator.truth_mask == 3UL);
    assert(reason == 22L);
}

static void test_reject_short_circuit(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;

    GEDER_PipelineInit(&pipeline);
    assert(GEDER_PipelineAddGate(&pipeline, gate_reject, 0, 0));
    assert(GEDER_PipelineAddGate(&pipeline, gate_accept_two, 0, 0));

    GEDER_RunBegin(&run, &pipeline, 0, 0);
    GEDER_RunExecute(&run);

    assert(run.final_result == GEDER_FINAL_REJECTED);
    assert(run.accumulator.gates_visited == 1u);
    assert(run.next_gate_index == 1u);
    assert(run.terminal_reason == 33L);
}

static void test_threshold_end_policy(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;

    GEDER_PipelineInit(&pipeline);
    assert(GEDER_PipelineSetEndPolicy(
        &pipeline,
        GEDER_END_ACCEPT_IF_MIN_TRUTHS_AND_SCORE,
        2u,
        GEDER_FixedFromInt(2L)
    ));
    assert(GEDER_PipelineAddGate(&pipeline, gate_continue_one, 0, 0));
    assert(GEDER_PipelineAddGate(&pipeline, gate_continue_one, 0, 0));

    GEDER_RunBegin(&run, &pipeline, 0, 0);
    GEDER_RunExecute(&run);

    assert(run.final_result == GEDER_FINAL_ACCEPTED);
    assert(run.accumulator.truths_accumulated == 2u);
}

static void test_disabled_gate(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;

    GEDER_PipelineInit(&pipeline);
    GEDER_PipelineSetEndPolicy(
        &pipeline,
        GEDER_END_ACCEPT_IF_ANY_TRUTH,
        0u,
        0L
    );
    assert(GEDER_PipelineAddGate(&pipeline, gate_reject, 0, 0));
    assert(GEDER_PipelineAddGate(&pipeline, gate_continue_one, 0, 0));
    assert(GEDER_PipelineSetGateEnabled(&pipeline, 0u, GEDER_FALSE));

    GEDER_RunBegin(&run, &pipeline, 0, 0);
    GEDER_RunExecute(&run);

    assert(run.final_result == GEDER_FINAL_ACCEPTED);
    assert(run.accumulator.gates_skipped == 1u);
    assert(run.accumulator.gates_visited == 1u);
}

static void test_incremental_execution(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;

    GEDER_PipelineInit(&pipeline);
    assert(GEDER_PipelineAddGate(&pipeline, gate_continue_one, 0, 0));
    assert(GEDER_PipelineAddGate(&pipeline, gate_accept_two, 0, 0));

    GEDER_RunBegin(&run, &pipeline, 0, 0);
    assert(GEDER_RunStep(&run) == GEDER_FINAL_PENDING);
    assert(run.accumulator.gates_visited == 1u);
    assert(!GEDER_RunIsFinished(&run));
    assert(GEDER_RunStep(&run) == GEDER_FINAL_ACCEPTED);
    assert(GEDER_RunIsFinished(&run));
}

int main(void)
{
    test_fixed_helpers();
    test_accept_chain();
    test_reject_short_circuit();
    test_threshold_end_policy();
    test_disabled_gate();
    test_incremental_execution();

    printf("GEDER Truth Gate Pipeline: all tests passed.\n");
    return 0;
}
