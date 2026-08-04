#include "geder_truth_gate.h"

static GEDER_Fixed geder_fixed_max_value(void)
{
    return (GEDER_Fixed)LONG_MAX;
}

static GEDER_Fixed geder_fixed_min_value(void)
{
    return (GEDER_Fixed)LONG_MIN;
}

static GEDER_GateVerdict geder_make_verdict(
    GEDER_Decision decision,
    int contributes_truth,
    GEDER_Fixed truth_delta,
    GEDER_TruthMask truth_mask,
    long reason_code
)
{
    GEDER_GateVerdict verdict;
    verdict.decision = decision;
    verdict.contributes_truth = contributes_truth ? GEDER_TRUE : GEDER_FALSE;
    verdict.truth_delta = truth_delta;
    verdict.truth_mask = truth_mask;
    verdict.reason_code = reason_code;
    return verdict;
}

static void geder_accumulator_reset(GEDER_Accumulator *accumulator)
{
    if (accumulator == 0)
    {
        return;
    }

    accumulator->gates_visited = 0u;
    accumulator->truths_accumulated = 0u;
    accumulator->gates_skipped = 0u;
    accumulator->truth_score = 0L;
    accumulator->truth_mask = GEDER_TRUTH_MASK_NONE;
    accumulator->last_reason = GEDER_REASON_NONE;
    accumulator->last_gate_index = 0u;
}

static void geder_accumulate_verdict(
    GEDER_Accumulator *accumulator,
    const GEDER_GateVerdict *verdict,
    unsigned int gate_index
)
{
    if (accumulator == 0 || verdict == 0)
    {
        return;
    }

    accumulator->gates_visited += 1u;
    accumulator->last_gate_index = gate_index;
    accumulator->last_reason = verdict->reason_code;

    if (verdict->contributes_truth)
    {
        accumulator->truths_accumulated += 1u;
        accumulator->truth_score = GEDER_FixedAddSaturating(
            accumulator->truth_score,
            verdict->truth_delta
        );
        accumulator->truth_mask |= verdict->truth_mask;
    }
}

static GEDER_FinalResult geder_resolve_end_policy(const GEDER_Run *run)
{
    const GEDER_PipelineConfig *config;
    const GEDER_Accumulator *accumulator;

    if (run == 0 || run->pipeline == 0)
    {
        return GEDER_FINAL_ERROR;
    }

    config = &run->pipeline->config;
    accumulator = &run->accumulator;

    switch (config->end_policy)
    {
        case GEDER_END_REJECT:
            return GEDER_FINAL_REJECTED;

        case GEDER_END_ACCEPT:
            return GEDER_FINAL_ACCEPTED;

        case GEDER_END_ACCEPT_IF_ANY_TRUTH:
            return accumulator->truths_accumulated > 0u
                ? GEDER_FINAL_ACCEPTED
                : GEDER_FINAL_REJECTED;

        case GEDER_END_ACCEPT_IF_MIN_TRUTHS:
            return accumulator->truths_accumulated >= config->minimum_truths
                ? GEDER_FINAL_ACCEPTED
                : GEDER_FINAL_REJECTED;

        case GEDER_END_ACCEPT_IF_SCORE:
            return accumulator->truth_score >= config->minimum_score
                ? GEDER_FINAL_ACCEPTED
                : GEDER_FINAL_REJECTED;

        case GEDER_END_ACCEPT_IF_MIN_TRUTHS_AND_SCORE:
            if (accumulator->truths_accumulated >= config->minimum_truths &&
                accumulator->truth_score >= config->minimum_score)
            {
                return GEDER_FINAL_ACCEPTED;
            }
            return GEDER_FINAL_REJECTED;

        default:
            return GEDER_FINAL_ERROR;
    }
}

GEDER_Fixed GEDER_FixedFromInt(long value)
{
    if (value > LONG_MAX / GEDER_FIXED_ONE)
    {
        return geder_fixed_max_value();
    }
    if (value < LONG_MIN / GEDER_FIXED_ONE)
    {
        return geder_fixed_min_value();
    }
    return (GEDER_Fixed)(value * GEDER_FIXED_ONE);
}

long GEDER_FixedToIntFloor(GEDER_Fixed value)
{
    long quotient;
    long remainder;

    quotient = value / GEDER_FIXED_ONE;
    remainder = value % GEDER_FIXED_ONE;

    if (value < 0L && remainder != 0L)
    {
        quotient -= 1L;
    }
    return quotient;
}

long GEDER_FixedToIntRound(GEDER_Fixed value)
{
    long quotient;
    long remainder;

    quotient = value / GEDER_FIXED_ONE;
    remainder = value % GEDER_FIXED_ONE;

    if (remainder >= GEDER_FIXED_HALF)
    {
        quotient += 1L;
    }
    else if (remainder <= -GEDER_FIXED_HALF)
    {
        quotient -= 1L;
    }
    return quotient;
}

GEDER_Fixed GEDER_FixedAddSaturating(GEDER_Fixed a, GEDER_Fixed b)
{
    if (b > 0L && a > LONG_MAX - b)
    {
        return geder_fixed_max_value();
    }
    if (b < 0L && a < LONG_MIN - b)
    {
        return geder_fixed_min_value();
    }
    return (GEDER_Fixed)(a + b);
}

GEDER_GateVerdict GEDER_VerdictReject(long reason_code)
{
    return geder_make_verdict(
        GEDER_DECISION_REJECT,
        GEDER_FALSE,
        0L,
        GEDER_TRUTH_MASK_NONE,
        reason_code
    );
}

GEDER_GateVerdict GEDER_VerdictAccept(
    int contributes_truth,
    GEDER_Fixed truth_delta,
    GEDER_TruthMask truth_mask,
    long reason_code
)
{
    return geder_make_verdict(
        GEDER_DECISION_ACCEPT,
        contributes_truth,
        truth_delta,
        truth_mask,
        reason_code
    );
}

GEDER_GateVerdict GEDER_VerdictContinue(
    int contributes_truth,
    GEDER_Fixed truth_delta,
    GEDER_TruthMask truth_mask,
    long reason_code
)
{
    return geder_make_verdict(
        GEDER_DECISION_CONTINUE,
        contributes_truth,
        truth_delta,
        truth_mask,
        reason_code
    );
}

GEDER_GateVerdict GEDER_VerdictError(long reason_code)
{
    return geder_make_verdict(
        GEDER_DECISION_ERROR,
        GEDER_FALSE,
        0L,
        GEDER_TRUTH_MASK_NONE,
        reason_code
    );
}

void GEDER_PipelineInit(GEDER_Pipeline *pipeline)
{
    if (pipeline == 0)
    {
        return;
    }

    GEDER_PipelineClear(pipeline);
    pipeline->config.end_policy = GEDER_END_REJECT;
    pipeline->config.minimum_truths = 1u;
    pipeline->config.minimum_score = GEDER_FIXED_ONE;
}

void GEDER_PipelineClear(GEDER_Pipeline *pipeline)
{
    unsigned int i;

    if (pipeline == 0)
    {
        return;
    }

    pipeline->gate_count = 0u;
    pipeline->config.end_policy = GEDER_END_REJECT;
    pipeline->config.minimum_truths = 1u;
    pipeline->config.minimum_score = GEDER_FIXED_ONE;

    for (i = 0u; i < GEDER_MAX_GATES; ++i)
    {
        pipeline->gates[i].function = 0;
        pipeline->gates[i].user_data = 0;
        pipeline->gates[i].parameters = 0;
        pipeline->gates[i].flags = 0u;
    }
}

int GEDER_PipelineSetEndPolicy(
    GEDER_Pipeline *pipeline,
    GEDER_EndPolicy policy,
    unsigned int minimum_truths,
    GEDER_Fixed minimum_score
)
{
    if (pipeline == 0)
    {
        return GEDER_FALSE;
    }
    if (policy < GEDER_END_REJECT ||
        policy > GEDER_END_ACCEPT_IF_MIN_TRUTHS_AND_SCORE)
    {
        return GEDER_FALSE;
    }

    pipeline->config.end_policy = policy;
    pipeline->config.minimum_truths = minimum_truths;
    pipeline->config.minimum_score = minimum_score;
    return GEDER_TRUE;
}

int GEDER_PipelineAddGate(
    GEDER_Pipeline *pipeline,
    GEDER_GateFunction function,
    void *user_data,
    const void *parameters
)
{
    GEDER_Gate *gate;

    if (pipeline == 0 || function == 0)
    {
        return GEDER_FALSE;
    }
    if (pipeline->gate_count >= GEDER_MAX_GATES)
    {
        return GEDER_FALSE;
    }

    gate = &pipeline->gates[pipeline->gate_count];
    gate->function = function;
    gate->user_data = user_data;
    gate->parameters = parameters;
    gate->flags = GEDER_GATE_ENABLED;
    pipeline->gate_count += 1u;
    return GEDER_TRUE;
}

int GEDER_PipelineSetGateEnabled(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    int enabled
)
{
    if (pipeline == 0 || gate_index >= pipeline->gate_count)
    {
        return GEDER_FALSE;
    }

    if (enabled)
    {
        pipeline->gates[gate_index].flags |= GEDER_GATE_ENABLED;
    }
    else
    {
        pipeline->gates[gate_index].flags &= ~GEDER_GATE_ENABLED;
    }
    return GEDER_TRUE;
}

int GEDER_PipelineSetGateParameters(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    const void *parameters
)
{
    if (pipeline == 0 || gate_index >= pipeline->gate_count)
    {
        return GEDER_FALSE;
    }
    pipeline->gates[gate_index].parameters = parameters;
    return GEDER_TRUE;
}

int GEDER_PipelineSetGateUserData(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    void *user_data
)
{
    if (pipeline == 0 || gate_index >= pipeline->gate_count)
    {
        return GEDER_FALSE;
    }
    pipeline->gates[gate_index].user_data = user_data;
    return GEDER_TRUE;
}

void GEDER_RunBegin(
    GEDER_Run *run,
    const GEDER_Pipeline *pipeline,
    const void *world,
    const void *statement
)
{
    if (run == 0)
    {
        return;
    }

    run->pipeline = pipeline;
    run->world = world;
    run->statement = statement;
    run->next_gate_index = 0u;
    geder_accumulator_reset(&run->accumulator);
    run->final_result = GEDER_FINAL_PENDING;
    run->finished = GEDER_FALSE;
    run->terminal_reason = GEDER_REASON_NONE;

    if (pipeline == 0)
    {
        run->final_result = GEDER_FINAL_ERROR;
        run->finished = GEDER_TRUE;
    }
}

GEDER_FinalResult GEDER_RunStep(GEDER_Run *run)
{
    const GEDER_Gate *gate;
    GEDER_GateInput input;
    GEDER_GateVerdict verdict;
    unsigned int index;

    if (run == 0 || run->pipeline == 0)
    {
        return GEDER_FINAL_ERROR;
    }
    if (run->finished)
    {
        return run->final_result;
    }

    while (run->next_gate_index < run->pipeline->gate_count)
    {
        index = run->next_gate_index;
        run->next_gate_index += 1u;
        gate = &run->pipeline->gates[index];

        if ((gate->flags & GEDER_GATE_ENABLED) == 0u)
        {
            run->accumulator.gates_skipped += 1u;
            continue;
        }
        if (gate->function == 0)
        {
            run->final_result = GEDER_FINAL_ERROR;
            run->finished = GEDER_TRUE;
            run->terminal_reason = -1L;
            return run->final_result;
        }

        input.world = run->world;
        input.statement = run->statement;
        input.parameters = gate->parameters;
        input.accumulator = &run->accumulator;
        input.gate_index = index;

        verdict = gate->function(&input, gate->user_data);
        geder_accumulate_verdict(&run->accumulator, &verdict, index);
        run->terminal_reason = verdict.reason_code;

        if (verdict.decision == GEDER_DECISION_REJECT)
        {
            run->final_result = GEDER_FINAL_REJECTED;
            run->finished = GEDER_TRUE;
        }
        else if (verdict.decision == GEDER_DECISION_ACCEPT)
        {
            run->final_result = GEDER_FINAL_ACCEPTED;
            run->finished = GEDER_TRUE;
        }
        else if (verdict.decision == GEDER_DECISION_ERROR)
        {
            run->final_result = GEDER_FINAL_ERROR;
            run->finished = GEDER_TRUE;
        }
        else if (verdict.decision != GEDER_DECISION_CONTINUE)
        {
            run->final_result = GEDER_FINAL_ERROR;
            run->finished = GEDER_TRUE;
            run->terminal_reason = -2L;
        }

        if (!run->finished)
        {
            while (run->next_gate_index < run->pipeline->gate_count &&
                   (run->pipeline->gates[run->next_gate_index].flags &
                    GEDER_GATE_ENABLED) == 0u)
            {
                run->accumulator.gates_skipped += 1u;
                run->next_gate_index += 1u;
            }

            if (run->next_gate_index >= run->pipeline->gate_count)
            {
                run->final_result = geder_resolve_end_policy(run);
                run->finished = GEDER_TRUE;
            }
        }

        return run->final_result;
    }

    run->final_result = geder_resolve_end_policy(run);
    run->finished = GEDER_TRUE;
    return run->final_result;
}

GEDER_FinalResult GEDER_RunExecute(GEDER_Run *run)
{
    GEDER_FinalResult result;

    if (run == 0)
    {
        return GEDER_FINAL_ERROR;
    }

    result = run->final_result;
    while (!run->finished)
    {
        result = GEDER_RunStep(run);
    }
    return result;
}

GEDER_FinalResult GEDER_PipelineEvaluate(
    const GEDER_Pipeline *pipeline,
    const void *world,
    const void *statement,
    GEDER_Accumulator *out_accumulator,
    long *out_terminal_reason
)
{
    GEDER_Run run;
    GEDER_FinalResult result;

    GEDER_RunBegin(&run, pipeline, world, statement);
    result = GEDER_RunExecute(&run);

    if (out_accumulator != 0)
    {
        *out_accumulator = run.accumulator;
    }
    if (out_terminal_reason != 0)
    {
        *out_terminal_reason = run.terminal_reason;
    }

    return result;
}

int GEDER_RunIsFinished(const GEDER_Run *run)
{
    if (run == 0)
    {
        return GEDER_TRUE;
    }
    return run->finished;
}

const GEDER_Accumulator *GEDER_RunGetAccumulator(const GEDER_Run *run)
{
    if (run == 0)
    {
        return 0;
    }
    return &run->accumulator;
}

const char *GEDER_FinalResultName(GEDER_FinalResult result)
{
    switch (result)
    {
        case GEDER_FINAL_PENDING:
            return "PENDING";
        case GEDER_FINAL_REJECTED:
            return "REJECTED";
        case GEDER_FINAL_ACCEPTED:
            return "ACCEPTED";
        case GEDER_FINAL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

const char *GEDER_DecisionName(GEDER_Decision decision)
{
    switch (decision)
    {
        case GEDER_DECISION_REJECT:
            return "REJECT";
        case GEDER_DECISION_ACCEPT:
            return "ACCEPT";
        case GEDER_DECISION_CONTINUE:
            return "CONTINUE";
        case GEDER_DECISION_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
