# Improvements (generic stdlib pass)

This pass focuses on *generic* (non-gameplay) FPSC-ish scripting support.

## Highlights

- **Activation binding**: adds `activated=X` (condition) and `activate=X` (action).
- **Numeric token resolver**:
  - Accepts numeric literals (`123`, `-5`, `3.14`)
  - Accepts **custom vars** like `%MyVar` when a `FPI_VarSystem` is wired
  - Accepts **internal vars** like `$ticks` when the host provides a resolver callback
- **Var comparisons** use an epsilon for equality (`FPI_VAR_EPSILON`, default `1e-9`).
- **Random semantics**: `random=X` returns true with probability `1/(X+1)`. `random=0` => always true.
- **randomize**: supports `randomize=Seed` (also via `%Var` / `$Internal`).
- **Parser**: RHS capture stops at `;` so inline comments like `state=1 ; comment` work.

## Demo CLI internal variables

The included `fpi_cli` demo wires a small internal resolver:

- `$ticks`  -> current tick index
- `$ms`     -> current time in ms
- `$timer`  -> elapsed ms since `timerstart`
- `$etimer` -> elapsed ms since `etimerstart`

(These are *demo-only*; embedder can define any internal names.)
