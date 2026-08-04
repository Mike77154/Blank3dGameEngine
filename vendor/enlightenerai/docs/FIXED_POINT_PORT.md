# Fixed-point port notes

This port removes all runtime floating point from EnlightenerAI.

## Converted

- `float` fields and function parameters were changed to `EAI_Fixed`.
- `sqrt` / `<math.h>` usage was replaced by `eai_fx_sqrt`.
- Vector scale, dot, length, distance, normalization, perception ranges, hearing ranges, A* costs, targeting scores, tactical scores, memory decay, and event values now use fixed-point helpers.
- The Makefile no longer links `-lm`.
- Tests and example code now pass fixed-point constants.

## Fixed format

`EAI_Fixed` is signed Q16.16:

- `EAI_FX_ONE` = 65536 raw
- `EAI_FX_FROM_INT(10)` = 10.0 fixed
- `EAI_FX_FROM_RAW(32768)` = 0.5 fixed

## Verification

Validated with:

```sh
make clean
make test
make examples/minimal_example
./examples/minimal_example
```

Result: all bundled tests passed.
