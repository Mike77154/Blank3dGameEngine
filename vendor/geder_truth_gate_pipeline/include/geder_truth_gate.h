#ifndef GEDER_TRUTH_GATE_H
#define GEDER_TRUTH_GATE_H

/*
 * GEDER Truth Gate Pipeline
 * C89, fixed storage, no heap, no floating point.
 *
 * The pipeline does not know what a "truth" means. Each gate evaluates an
 * opaque world/context and returns one of three decisions:
 *   REJECT   - stop immediately and reject the statement.
 *   ACCEPT   - stop immediately and accept the statement as absolute truth.
 *   CONTINUE - accumulate this gate's evidence and ask the next gate.
 */

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if LONG_MAX < 2147483647L
#error GEDER requires a signed long type with at least 32 bits.
#endif

#ifndef GEDER_MAX_GATES
#define GEDER_MAX_GATES 32
#endif

#ifndef GEDER_FIXED_SHIFT
#define GEDER_FIXED_SHIFT 16
#endif

#define GEDER_FIXED_ONE ((long)(1L << GEDER_FIXED_SHIFT))
#define GEDER_FIXED_HALF ((long)(GEDER_FIXED_ONE >> 1))

#define GEDER_TRUE 1
#define GEDER_FALSE 0

#define GEDER_GATE_ENABLED 1u

#define GEDER_REASON_NONE 0L
#define GEDER_TRUTH_MASK_NONE 0UL

typedef long GEDER_Fixed;
typedef unsigned long GEDER_TruthMask;

typedef enum GEDER_Decision
{
    GEDER_DECISION_REJECT = 0,
    GEDER_DECISION_ACCEPT = 1,
    GEDER_DECISION_CONTINUE = 2,
    GEDER_DECISION_ERROR = 3
} GEDER_Decision;

typedef enum GEDER_FinalResult
{
    GEDER_FINAL_PENDING = 0,
    GEDER_FINAL_REJECTED = 1,
    GEDER_FINAL_ACCEPTED = 2,
    GEDER_FINAL_ERROR = 3
} GEDER_FinalResult;

typedef enum GEDER_EndPolicy
{
    GEDER_END_REJECT = 0,
    GEDER_END_ACCEPT = 1,
    GEDER_END_ACCEPT_IF_ANY_TRUTH = 2,
    GEDER_END_ACCEPT_IF_MIN_TRUTHS = 3,
    GEDER_END_ACCEPT_IF_SCORE = 4,
    GEDER_END_ACCEPT_IF_MIN_TRUTHS_AND_SCORE = 5
} GEDER_EndPolicy;

typedef struct GEDER_Accumulator
{
    unsigned int gates_visited;
    unsigned int truths_accumulated;
    unsigned int gates_skipped;
    GEDER_Fixed truth_score;
    GEDER_TruthMask truth_mask;
    long last_reason;
    unsigned int last_gate_index;
} GEDER_Accumulator;

typedef struct GEDER_GateInput
{
    const void *world;
    const void *statement;
    const void *parameters;
    const GEDER_Accumulator *accumulator;
    unsigned int gate_index;
} GEDER_GateInput;

typedef struct GEDER_GateVerdict
{
    GEDER_Decision decision;
    int contributes_truth;
    GEDER_Fixed truth_delta;
    GEDER_TruthMask truth_mask;
    long reason_code;
} GEDER_GateVerdict;

typedef GEDER_GateVerdict (*GEDER_GateFunction)(
    const GEDER_GateInput *input,
    void *gate_user_data
);

typedef struct GEDER_Gate
{
    GEDER_GateFunction function;
    void *user_data;
    const void *parameters;
    unsigned int flags;
} GEDER_Gate;

typedef struct GEDER_PipelineConfig
{
    GEDER_EndPolicy end_policy;
    unsigned int minimum_truths;
    GEDER_Fixed minimum_score;
} GEDER_PipelineConfig;

typedef struct GEDER_Pipeline
{
    GEDER_Gate gates[GEDER_MAX_GATES];
    unsigned int gate_count;
    GEDER_PipelineConfig config;
} GEDER_Pipeline;

typedef struct GEDER_Run
{
    const GEDER_Pipeline *pipeline;
    const void *world;
    const void *statement;
    unsigned int next_gate_index;
    GEDER_Accumulator accumulator;
    GEDER_FinalResult final_result;
    int finished;
    long terminal_reason;
} GEDER_Run;

/* Fixed-point helpers. They never use float or double. */
GEDER_Fixed GEDER_FixedFromInt(long value);
long GEDER_FixedToIntFloor(GEDER_Fixed value);
long GEDER_FixedToIntRound(GEDER_Fixed value);
GEDER_Fixed GEDER_FixedAddSaturating(GEDER_Fixed a, GEDER_Fixed b);

/* Verdict constructors. */
GEDER_GateVerdict GEDER_VerdictReject(long reason_code);
GEDER_GateVerdict GEDER_VerdictAccept(
    int contributes_truth,
    GEDER_Fixed truth_delta,
    GEDER_TruthMask truth_mask,
    long reason_code
);
GEDER_GateVerdict GEDER_VerdictContinue(
    int contributes_truth,
    GEDER_Fixed truth_delta,
    GEDER_TruthMask truth_mask,
    long reason_code
);
GEDER_GateVerdict GEDER_VerdictError(long reason_code);

/* Pipeline configuration. */
void GEDER_PipelineInit(GEDER_Pipeline *pipeline);
void GEDER_PipelineClear(GEDER_Pipeline *pipeline);
int GEDER_PipelineSetEndPolicy(
    GEDER_Pipeline *pipeline,
    GEDER_EndPolicy policy,
    unsigned int minimum_truths,
    GEDER_Fixed minimum_score
);
int GEDER_PipelineAddGate(
    GEDER_Pipeline *pipeline,
    GEDER_GateFunction function,
    void *user_data,
    const void *parameters
);
int GEDER_PipelineSetGateEnabled(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    int enabled
);
int GEDER_PipelineSetGateParameters(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    const void *parameters
);
int GEDER_PipelineSetGateUserData(
    GEDER_Pipeline *pipeline,
    unsigned int gate_index,
    void *user_data
);

/* Incremental execution. One call to GEDER_RunStep evaluates at most one gate. */
void GEDER_RunBegin(
    GEDER_Run *run,
    const GEDER_Pipeline *pipeline,
    const void *world,
    const void *statement
);
GEDER_FinalResult GEDER_RunStep(GEDER_Run *run);
GEDER_FinalResult GEDER_RunExecute(GEDER_Run *run);
GEDER_FinalResult GEDER_PipelineEvaluate(
    const GEDER_Pipeline *pipeline,
    const void *world,
    const void *statement,
    GEDER_Accumulator *out_accumulator,
    long *out_terminal_reason
);

/* Read-only helpers. */
int GEDER_RunIsFinished(const GEDER_Run *run);
const GEDER_Accumulator *GEDER_RunGetAccumulator(const GEDER_Run *run);
const char *GEDER_FinalResultName(GEDER_FinalResult result);
const char *GEDER_DecisionName(GEDER_Decision decision);

#ifdef __cplusplus
}
#endif

#endif /* GEDER_TRUTH_GATE_H */
