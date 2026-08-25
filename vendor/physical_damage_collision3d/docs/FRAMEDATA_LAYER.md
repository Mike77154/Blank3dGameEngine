# PDC3D Frame Data Layer

`pdc3d_framedata.h` wraps the vendored `fd3d89` kernel so game code can stay on the PDC3D API surface.

## Role

This layer does **not** replace the PDC3D hitbox/hurtbox resolver. It only adds deterministic combat timing metadata:

- startup / active / recovery action frames
- i-frames and hit filters
- side-step evasion checks
- hitstop / blockstop
- hitstun / blockstun
- cancel masks
- counter-window flags
- frame advantage queries

## Protocol

- C89
- fixed-point only
- no malloc/calloc/realloc/free
- no heap ownership
- no float/double
- static user-authored action data

## Pipeline

```txt
PDC3D hitbox/hurtbox contact
        ↓
PDC3D frame-data gate
        ↓
PDC3D defense layer
        ↓
PDC3D frame-data contact timers
        ↓
weakspot/material/final damage event
```

## Fixed-point bridge

PDC3D uses the hurtbox kernel fixed-point scale. `fd3d89` uses `FD3D_FX_SHIFT = 12`.
Use these bridge helpers instead of mixing the types directly:

```c
FD3D_Fx pdc3d_fd_from_pdc_fx(hb3_fx v);
hb3_fx  pdc3d_fd_to_pdc_fx(FD3D_Fx v);
```

## Engine-facing functions

Use the `pdc3d_fd_*` functions from `pdc3d.h` for world-level integration.
Use the lower-level `pdc3d_framedata_*` functions only when building another adapter.
