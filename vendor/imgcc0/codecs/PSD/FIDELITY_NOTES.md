# FIDELITY NOTES

## v2.2 focus

`v2.2` keeps the deeper file-fidelity work from `v2.1`, but moves the main implementation focus to **live classic-effect rendering** inside the fixed-point compositor.

Implemented, live, and tested in the raster path:

- `dsdw` drop shadow
- `isdw` inner shadow
- `oglw` outer glow
- `iglw` inner glow
- `bevl` bevel-lite
- `sofi` solid fill

## Rendering model

The compositor remains:

- ANSI C / C89
- static-only / no heap in the public API
- fixed-point `Q16.16`
- scanline-oriented

To stay deterministic and heap-free, the new `lrFX` path uses conservative **row-based raster approximations**:

- horizontal drop-shadow offset derived from effect angle sign
- local blur radius capped to a small fixed scanline neighborhood
- outer glow sampled from nearby row coverage
- inner glow sampled from inside-edge proximity on the current row
- bevel-lite derived from local edge contrast on the current row

## What this means

`v2.2` is better for:

- predictable raster pipelines
- regression-tested classic-style previews
- authored PSD roundtrip where the classic `lrFX` metadata is already preserved

`v2.2` still does **not** try to be a pixel-identical Photoshop renderer for:

- exact blur kernels
- full 2D light vectors and spread/choke semantics
- object-based modern effect stacks
- complex text / Smart Object rendering

## Regression anchor

`tests/test_v22` covers the live effect path with deterministic expected pixels for:

- drop shadow
- outer glow
- inner shadow
- inner glow
- bevel-lite
