# condor_evact89 v0.2

Context-based C89 Event -> Condition -> Action rule runtime derived from the
uploaded `condor_evact_lib` design.

## Engine-ready changes

- no global singleton tables; caller owns `cea89_context`
- fixed capacities configurable at compile time
- generational handles preserved across reset
- listener/rule snapshot dispatch remains mutation-safe
- event carries both gameplay `owner` and Thing/variable `instance`
- no heap, no float/double, no libm dependency

`legacy/condor_evact_lib` preserves the uploaded source snapshot. Blank3D builds
only `include/condor_evact89.h` + `src/condor_evact89.c`.
