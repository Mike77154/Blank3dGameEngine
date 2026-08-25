# FD3D protocol vendor replacement

This package replaces the PDC3D v3 FD3D vendor with the protocol-patched `fdata3d89` drop.

## Why this is better

The patched FD3D vendor is API-compatible with the previous vendor, but fixes two protocol-level issues:

- `FD3D_FX_FROM_INT(v)` no longer left-shifts signed input. It uses fixed-point multiplication instead.
- `FD3D_ArenaAlloc` now treats `align == 0` as default alignment and rejects non-power-of-two non-zero alignments.

## Extra PDC3D-side hardening

Because PDC3D also converts negative world-space coordinates between fixed-point scales, the wrapper was hardened too:

- `pdc3d_fd_from_pdc_fx` and `pdc3d_fd_to_pdc_fx` now use multiply/divide conversions instead of signed left shifts.
- `pdc3d_defense` and `pdc3d` bridge conversion helpers now avoid signed left shifts for scale changes.
- `PDC3D_FX_FROM_INT` was changed to multiplication.

## What did not change

This does not replace PDC3D's contact resolver, melee resolver, defense layer, weakspot layer, grab layer, or throw layer. FD3D remains an internal frame-data gate used by `pdc3d_framedata.h`.

## New regression demo

`examples/demo_pdc3d_fd3d_protocol_patch.c` checks:

- arena default alignment with `align == 0`
- rejection of bad alignment such as `3`
- accepted power-of-two alignment such as `8`
- negative fixed-point conversion roundtrip through PDC3D <-> FD3D
