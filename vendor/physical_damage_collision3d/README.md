# PhysicalDamageCollision3D / PDC3D

C89 fixed-point combat collision and physical damage kernel for 3D engines.

## v3 layers

```txt
PDC3D v1: hitbox / hurtbox / melee / grab / throw / weakspot
PDC3D v2: defense layer: block / guard / parry / shell / counter
PDC3D v3: frame-data layer: actions / i-frames / hitstop / stun / frame advantage
```

## Protocol

- C89
- fixed-point only
- no malloc
- no calloc
- no realloc
- no free
- no float/double
- static arrays and caller-owned arenas
- engine agnostic

## Public headers

```txt
include/pdc3d.h             main facade
include/pdc3d_damage.h      damage packet/material rules
include/pdc3d_defense.h     defense pipeline wrapper
include/pdc3d_framedata.h   frame-data wrapper over vendored fd3d89
```

## Build

```sh
make clean
make all
make run
```

## Important design rule

`fd3d89` is vendored, but the engine should use `pdc3d_framedata.h` or the `pdc3d_fd_*` functions exposed by `pdc3d.h`.
Do not mix FD3D fixed-point directly with PDC3D fixed-point. Use:

```c
FD3D_Fx pdc3d_fd_from_pdc_fx(hb3_fx v);
hb3_fx  pdc3d_fd_to_pdc_fx(FD3D_Fx v);
```

## Damage pipeline

```txt
PDC3D contact resolver
        ↓
frame-data gate: active frame / i-frame / side-step / counter window
        ↓
defense layer: parry / guard / block / shell / counter token
        ↓
frame-data contact timers: hitstop / hitstun / blockstop / blockstun
        ↓
weakspot/material/final damage event
```

## New v3 demo

`examples/demo_pdc3d_framedata_layer.c` checks:

- strike i-frame filtering
- FD action hitdef damage handoff
- hitstop/hitstun metadata
- blockstop/blockstun metadata
- frame advantage calculation

## v3.1 protocol vendor replacement

This build replaces the internal FD3D vendor with `fdata3d89_protocol_patched`. The FD3D public surface is still hidden behind `pdc3d_framedata.h`; the engine should continue talking to PDC3D only.

Protocol fixes:

- safer fixed-point conversion from integers, avoiding signed left-shift on negative values
- stricter arena alignment validation
- `align == 0` uses default alignment
- PDC3D bridge conversions hardened for negative world-space coordinates

New regression demo:

```sh
make run
```

Includes `demo_pdc3d_fd3d_protocol_patch`.
