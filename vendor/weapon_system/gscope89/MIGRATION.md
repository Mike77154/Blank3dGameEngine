# Migration notes v0.2

The reticle pack has crossed the hardcode boundary.

Before:

```text
gscopepresets89.c
  -> 192 registry rows
  -> 3,953 compiled gsv89_shape initializers
```

Now:

```text
active.ini -> catalog.ini -> presets/<formal-reticle>.ini -> gscopepresets89 loader -> gscopevector89
```

`gscopepresets89.c` contains only parsing, fixed-storage catalog management, palette defaults and emission glue. It contains no concrete reticle geometry.

The old tables live only in `modules/gscopepresets89/legacy_reference/` so the migration can be verified. `PARITY_192.txt` proves the baseline INI data reconstructs the same 192 presets and 3,953 shapes.

Adding a reticle requires no C change: add one formal preset INI and one catalog line.

## v0.3 provider migration

Existing direct module calls remain valid. Provider support is opt-in through `gscopebundle89`.

Recommended integration path:

1. Keep existing `gsp89_emit_cb` / `gsh89_emit_cb` sinks.
2. Replace `gsp89_painter_init()` with `gscb89_painter_init()` only where provider routing is desired.
3. Register a `gpr89_provider` with the subset of callbacks the engine owns.
4. Include `config/providers/plug_and_play.ini` or set routes directly.
5. Replace direct `gsv89_emit_shapes`, `gsr89_emit_layer`, `gsb89_emit_bar`, or preset emission with the `gscb89_*` facade calls as needed.
6. Leave any unowned domain in `internal` or `auto` mode; the original implementation remains the fallback.

No all-at-once migration is required.
