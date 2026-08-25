# FIDELITY NOTES v2.3

## Focus

`v2.3` keeps the live classic-`lrFX` path from `v2.2`, but hardens the raster behavior for multi-row content.

The main change is that the compositor is no longer limited to single-row effect neighborhoods for the classic effect families that benefit most from blur-like falloff:

- `dsdw` drop shadow
- `oglw` outer glow
- `isdw` inner shadow
- `iglw` inner glow
- `bevl` bevel-lite

## Rendering model

The raster path is still:

- ANSI C / C89
- public API remains static-only / no public heap allocation
- fixed-point `Q16.16`
- stream-oriented compose path

What changed is the local effect evaluation:

- **drop shadow / outer glow** now evaluate a conservative **2D weighted neighborhood max** on multi-row content
- **inner shadow / inner glow** now use a more serious **inner-edge proximity search** on multi-row content
- **bevel-lite** now derives highlight/shadow from a conservative **2D edge gradient** instead of only left/right contrast
- the classic stored **`intensity`** field is used as a **spread-like / choke-like shaping control** for larger kernels

## Compatibility choices

To avoid breaking earlier regression anchors, the old single-row path is still favored where that keeps behavior stable:

- one-row layers keep the tighter row-based path for inner effects and bevel-lite
- the public file format fields for classic `lrFX` are unchanged
- the improvement is mainly in the **live compositor**, not in the stored tag structure

## What this does not claim

`v2.3` still does **not** claim Photoshop-exact blur kernels, contour math, or layer-style interaction semantics.

The goal is:

- better visual plausibility for multi-row content
- deterministic fixed-point behavior
- no new public heap requirement
- preserved PSD roundtrip for the classic effect metadata that already existed in `v2.2`
