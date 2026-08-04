# Architecture

EnlightenerAI Fixed is split into reusable service modules.

## Numeric model

The whole runtime uses Q16.16 fixed point through `EAI_Fixed`.

- Positions: `EAI_Vec3` with `EAI_Fixed x/y/z`
- Time: `EAI_Fixed dt`
- Costs / scores / ranges / event values: `EAI_Fixed`
- Math helpers: `eai_fx_mul`, `eai_fx_div`, `eai_fx_sqrt`, vector dot/length/distance/normalize
- No standard floating-point math or `<math.h>` is used.

## Memory model

Everything lives inside `EAI_Context` using fixed-size pools:

- entities
- nav nodes
- nav edges
- zones
- stimuli
- action intents
- A* records
- event ring buffer

The caller owns the context object. EnlightenerAI does not allocate memory dynamically.

## Core loop

1. Sync entity transform data from the engine.
2. `eai_update()`
   - perception
   - memory decay / refresh
   - targeting
3. Consume events or dispatch them into your FSM.
4. Query nav / tactical services from the FSM.
5. Convert `EAI_ActionIntent` into engine-side movement or attack commands.

## Main modules

- `eai_nav.*`
  - fixed graph
  - A*
  - nearest node
  - path smoothing
  - zones and zone locks
- `eai_perception.*`
  - vision cone
  - hearing stimuli
  - event emission
- `eai_targeting.*`
  - hostility matrix
  - target scoring
  - target acquisition / loss events
- `eai_tactical.*`
  - cover query
  - flank query
  - exposure test
- `eai_memory.*`
  - alertness
  - suspicion
  - last seen / heard positions
- `eai_actions.*`
  - output intent layer
- `eai_fsm_adapter.*`
  - event dispatch helper
