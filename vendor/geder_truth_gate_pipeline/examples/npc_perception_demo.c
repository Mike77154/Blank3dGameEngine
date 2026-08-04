#include <stdio.h>
#include "geder_truth_gate.h"

#define TRUTH_NEAR       0x0001UL
#define TRUTH_IN_CONE    0x0002UL
#define TRUTH_CLEAR_LOS  0x0004UL

#define REASON_TOO_FAR       1001L
#define REASON_NEAR          1002L
#define REASON_OUTSIDE_CONE  2001L
#define REASON_INSIDE_CONE   2002L
#define REASON_BLOCKED       3001L
#define REASON_VISIBLE       3002L

typedef struct DemoWorld
{
    long distance_units;
    int inside_vision_cone;
    int line_of_sight_clear;
} DemoWorld;

typedef struct DistanceParams
{
    long maximum_distance_units;
} DistanceParams;

static GEDER_GateVerdict gate_distance(
    const GEDER_GateInput *input,
    void *gate_user_data
)
{
    const DemoWorld *world;
    const DistanceParams *params;

    (void)gate_user_data;
    world = (const DemoWorld *)input->world;
    params = (const DistanceParams *)input->parameters;

    if (world == 0 || params == 0)
    {
        return GEDER_VerdictError(9001L);
    }

    if (world->distance_units > params->maximum_distance_units)
    {
        return GEDER_VerdictReject(REASON_TOO_FAR);
    }

    return GEDER_VerdictContinue(
        GEDER_TRUE,
        GEDER_FixedFromInt(1L),
        TRUTH_NEAR,
        REASON_NEAR
    );
}

static GEDER_GateVerdict gate_vision_cone(
    const GEDER_GateInput *input,
    void *gate_user_data
)
{
    const DemoWorld *world;

    (void)gate_user_data;
    world = (const DemoWorld *)input->world;

    if (world == 0)
    {
        return GEDER_VerdictError(9002L);
    }
    if (!world->inside_vision_cone)
    {
        return GEDER_VerdictReject(REASON_OUTSIDE_CONE);
    }

    return GEDER_VerdictContinue(
        GEDER_TRUE,
        GEDER_FixedFromInt(2L),
        TRUTH_IN_CONE,
        REASON_INSIDE_CONE
    );
}

static GEDER_GateVerdict gate_line_of_sight(
    const GEDER_GateInput *input,
    void *gate_user_data
)
{
    const DemoWorld *world;

    (void)gate_user_data;
    world = (const DemoWorld *)input->world;

    if (world == 0)
    {
        return GEDER_VerdictError(9003L);
    }
    if (!world->line_of_sight_clear)
    {
        return GEDER_VerdictReject(REASON_BLOCKED);
    }

    return GEDER_VerdictAccept(
        GEDER_TRUE,
        GEDER_FixedFromInt(4L),
        TRUTH_CLEAR_LOS,
        REASON_VISIBLE
    );
}

static void print_run(const char *label, GEDER_Run *run)
{
    const GEDER_Accumulator *accumulator;

    accumulator = GEDER_RunGetAccumulator(run);
    printf("%s\n", label);
    printf("  result: %s\n", GEDER_FinalResultName(run->final_result));
    printf("  gates visited: %u\n", accumulator->gates_visited);
    printf("  truths: %u\n", accumulator->truths_accumulated);
    printf("  score: %ld\n", GEDER_FixedToIntFloor(accumulator->truth_score));
    printf("  mask: 0x%04lX\n", accumulator->truth_mask);
    printf("  reason: %ld\n\n", run->terminal_reason);
}

int main(void)
{
    GEDER_Pipeline pipeline;
    GEDER_Run run;
    DistanceParams distance_params;
    DemoWorld visible_world;
    DemoWorld blocked_world;
    DemoWorld far_world;

    distance_params.maximum_distance_units = 20L;

    GEDER_PipelineInit(&pipeline);
    GEDER_PipelineSetEndPolicy(
        &pipeline,
        GEDER_END_REJECT,
        0u,
        0L
    );

    GEDER_PipelineAddGate(&pipeline, gate_distance, 0, &distance_params);
    GEDER_PipelineAddGate(&pipeline, gate_vision_cone, 0, 0);
    GEDER_PipelineAddGate(&pipeline, gate_line_of_sight, 0, 0);

    visible_world.distance_units = 10L;
    visible_world.inside_vision_cone = GEDER_TRUE;
    visible_world.line_of_sight_clear = GEDER_TRUE;

    blocked_world.distance_units = 10L;
    blocked_world.inside_vision_cone = GEDER_TRUE;
    blocked_world.line_of_sight_clear = GEDER_FALSE;

    far_world.distance_units = 50L;
    far_world.inside_vision_cone = GEDER_TRUE;
    far_world.line_of_sight_clear = GEDER_TRUE;

    GEDER_RunBegin(&run, &pipeline, &visible_world, 0);
    GEDER_RunExecute(&run);
    print_run("Visible player", &run);

    GEDER_RunBegin(&run, &pipeline, &blocked_world, 0);
    GEDER_RunExecute(&run);
    print_run("Blocked player", &run);

    GEDER_RunBegin(&run, &pipeline, &far_world, 0);
    GEDER_RunExecute(&run);
    print_run("Far player", &run);

    return 0;
}
