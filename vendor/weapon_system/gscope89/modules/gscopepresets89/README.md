# gscopepresets89 - INI-backed edition

`gscopepresets89` renders pure-vector reticles, but it no longer compiles the reticle catalog or geometry into C tables.

Runtime content comes from:

```text
../../config/reticles/catalog.ini
../../config/reticles/presets/*.ini
```

The baseline pack contains 192 presets and 3,953 vector shapes. The original numeric IDs remain ABI-compatible, while the runtime catalog can grow beyond 192 without adding enum values.

## Core API

```c
gsvp89_set_catalog_path("config/reticles/catalog.ini");
if (!gsvp89_reload()) {
    /* inspect gsvp89_last_error() */
}

preset = gsvp89_find("svd_pso1_dragunov");
gsvp89_emit(&painter, preset, 0, 255);
```

Or select through another INI:

```c
preset = gsvp89_select_from_ini("config/reticles/active.ini", "reticle");
```

`active.ini` contains only `catalog=...` and `use=...`.

## Fixed-storage limits

No malloc/realloc/free/heap is used. Defaults:

- 512 presets
- 8192 total shapes
- recursive preset includes up to depth 8

The baseline uses 192 presets / 3,953 shapes.

## Validation

```sh
make test
```

The runtime loader is compiled with C89 pedantic warnings-as-errors, loads all 192 formal INIs, renders every preset, and verifies selector lookup. `PARITY_192.txt` records the full geometry/metadata hash match against the old C tables.

`legacy_reference/` exists only to prove migration parity and is not a runtime dependency.
