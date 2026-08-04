# EnlightenerAI Fixed

EnlightenerAI is a C89, fixed-point AI middleware package that gives a finite-state machine better world-reading tools:

- A* pathfinding on a fixed navigation graph
- zones / containers via node and entity zone locks
- vision and hearing
- target selection
- tactical cover / flank queries
- tactical memory
- event queue
- action intents
- lightweight FSM event dispatch helper

It is **not** a behavior system by itself. The FSM still decides. EnlightenerAI feeds the FSM with better information.

## Design rules

- C89 / `-std=c89 -Wall -Wextra -pedantic`
- no dynamic allocation
- no recursion
- no `float` / no `double`
- no `<math.h>` dependency
- no `-lm` link step
- Q16.16 fixed-point scalar type: `EAI_Fixed`
- fixed-size pools / arena-style ownership inside `EAI_Context`
- no engine dependency; world queries are provided through callbacks

## Fixed-point notes

Coordinates, costs, time deltas, view ranges, hearing ranges, target scores, event values, and tactical scores now use `EAI_Fixed`.

Useful helpers live in `include/eai_math.h`:

```c
EAI_Fixed eai_fx_from_int(EAI_Fixed value);
EAI_Fixed eai_fx_from_ratio(EAI_Fixed numerator, EAI_Fixed denominator);
EAI_Fixed eai_fx_mul(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_div(EAI_Fixed a, EAI_Fixed b);
EAI_Fixed eai_fx_sqrt(EAI_Fixed value);
```

For compile-time constants, use the raw Q16.16 helpers from `include/eai_config.h`:

```c
EAI_FX_ONE              /* 1.0 */
EAI_FX_ZERO             /* 0.0 */
EAI_FX_FROM_INT(5)      /* 5.0 */
EAI_FX_FROM_RAW(32768)  /* 0.5 */
```

## Status

This package is a working fixed-point foundation:

- implemented: core, events, tactical memory, nav graph, A*, path smoothing, zones, vision, hearing, targeting, cover query, flank query, action intents, debug summaries, FSM event dispatcher
- intentionally lightweight: no animation system, no steering solver, no squad planner, no editor UI

## Build

```sh
make
make test
```

No math library is linked.

## Quick integration flow

1. Fill `EAI_WorldOps` with your raycast / cost callbacks.
2. Call `eai_init(&ctx, &ops, user_ptr);`
3. Create nav nodes and edges using fixed-point positions / costs.
4. Create entities.
5. On each frame:
   - sync fixed-point positions / forwards from your engine
   - call `eai_update(&ctx, dt);`
   - dispatch or pop events
   - let your FSM query nav / perception / tactical modules
   - translate `EAI_ActionIntent` into engine-side movement / attack commands

## Example

See `examples/minimal_example.c`.


## Hardening notes

This package was hardened for the no-heap fixed-point protocol:

- Runtime code avoids `float`, `double`, `malloc`, `calloc`, `realloc`, `free`, and `<math.h>`.
- `EAI_Fixed` and `EAI_U32` are selected as real 32-bit types using C89 `<limits.h>` guards.
- Fixed-point accumulation now uses saturating helpers for risky add/sub/multiply-add paths.
- A*, targeting, perception, memory timers, vector math, and tactical scoring were patched to avoid signed overflow.
- Bidirectional nav edge insertion now fails before writing if two edge slots are not available.
- Default Makefile flags use strict C89 and `-ffreestanding -fno-builtin`.

