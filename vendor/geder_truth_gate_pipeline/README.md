# GEDER Truth Gate Pipeline

**GEDER** is a small, agnostic C89 library that controls **how a statement becomes accepted as truth**.

It does not know what a player, NPC, raycast, sensor, rule, permission, diagnosis, quest, or combat condition is. It only executes ordered gates and manages their verdicts.

```text
Gate 1
  REJECT   -> false final
  ACCEPT   -> true final
  CONTINUE -> accumulate truth and ask Gate 2

Gate 2
  REJECT   -> false final
  ACCEPT   -> true final
  CONTINUE -> accumulate truth and ask Gate 3
```

## Design rules

- ISO C89 source style.
- No `malloc`, `calloc`, `realloc`, `free`, or hidden heap use.
- No `float` or `double`.
- Fixed-point score (`Q16.16` by default).
- Caller-owned world, statement, parameters, and provider data.
- Fixed-capacity gate array (`GEDER_MAX_GATES`, default 32).
- Full evaluation or one-gate-per-step incremental execution.
- No engine dependency.

## What a gate returns

Each gate returns one decision:

- `GEDER_DECISION_REJECT`: parameter failed; stop immediately.
- `GEDER_DECISION_ACCEPT`: enough truth has been accumulated; stop and accept.
- `GEDER_DECISION_CONTINUE`: this parameter passed; accumulate its evidence and ask the next gate.
- `GEDER_DECISION_ERROR`: malformed provider/input; stop with error.

A successful gate may contribute:

- one accumulated truth;
- a fixed-point score delta;
- one or more bits in a truth mask;
- a reason code for debugging or gameplay logic.

## End-of-chain policies

When every gate returns `CONTINUE`, GEDER can resolve the statement using:

- reject;
- accept;
- accept if any truth accumulated;
- accept if a minimum truth count was reached;
- accept if a fixed-point score was reached;
- accept if both count and score were reached.

## Typical NPC perception stack

```text
World knowledge
    -> distance gate
    -> orientation gate
    -> eye-cone gate
    -> line-of-sight gate
    -> ACCEPT
```

A retro enemy may put an immediate `ACCEPT` in the first gate. A modern guard may use the whole chain. GEDER only administers the order and the accumulation.

## Minimal use

```c
GEDER_Pipeline pipeline;
GEDER_Run run;

GEDER_PipelineInit(&pipeline);
GEDER_PipelineAddGate(&pipeline, gate_distance, 0, &distance_params);
GEDER_PipelineAddGate(&pipeline, gate_cone, 0, &cone_params);
GEDER_PipelineAddGate(&pipeline, gate_los, 0, &los_params);

GEDER_RunBegin(&run, &pipeline, &world, &statement);
GEDER_RunExecute(&run);

if (run.final_result == GEDER_FINAL_ACCEPTED)
{
    /* The statement is now accepted as absolute truth. */
}
```

## Incremental execution

`GEDER_RunStep()` evaluates at most one enabled gate. This lets an engine distribute expensive validation across frames.

```c
GEDER_RunBegin(&run, &pipeline, world, statement);

while (!GEDER_RunIsFinished(&run))
{
    GEDER_RunStep(&run);
}
```

In a real-time engine, call `GEDER_RunStep()` once per update instead of using a loop.

## Build

GCC / MinGW:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude \
    src/geder_truth_gate.c examples/npc_perception_demo.c \
    -o npc_perception_demo
```

Tests:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude \
    src/geder_truth_gate.c tests/test_geder_truth_gate.c \
    -o test_geder_truth_gate
./test_geder_truth_gate
```

## Files

```text
GEDER_Truth_Gate_Pipeline/
├── include/
│   └── geder_truth_gate.h
├── src/
│   └── geder_truth_gate.c
├── examples/
│   ├── npc_perception_demo.c
│   └── incremental_demo.c
├── tests/
│   └── test_geder_truth_gate.c
├── Makefile
├── LICENSE.txt
└── README.md
```
