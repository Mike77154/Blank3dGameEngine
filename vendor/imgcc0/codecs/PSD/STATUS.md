# STATUS

## v2.3 delivered

- multi-row **2D neighborhood kernels** for live classic `lrFX`:
  - `dsdw`
  - `oglw`
- more serious **choke-like / spread-like shaping** for classic live effects using the stored `intensity` field on larger kernels
- multi-row **2D bevel-lite** edge-gradient shading
- `test_v23` coverage for:
  - vertical/diagonal shadow-glow falloff
  - stronger edge-vs-center inner glow behavior
  - corner-biased bevel shading
- `examples/fidelity_roundtrip_v23` and `examples/example_fidelity_roundtrip_v23.psd`

## v2.2 delivered

- conservative live classic-`lrFX` raster rendering in the compositor for:
  - `dsdw`
  - `isdw`
  - `oglw`
  - `iglw`
  - `bevl`
  - `sofi`
- `test_v22` coverage for live effect output
- `examples/fidelity_roundtrip` and `examples/example_fidelity_roundtrip.psd`

## v2.1 delivered

- ANSI C / C89
- static-only memory policy
- seekable stream I/O
- big-endian readers/writers
- Gray / RGB, 8 bpc
- RAW + RLE + ZIP + ZIP+prediction
- composite read/write
- layer metadata read
- on-demand per-layer channel decode
- fixed-point `Q16.16` compositor
- clipping-base support
- merged alpha write
- base user layer masks (`-2`) parse/write/compose
- file stdio helpers
- on-disk corpus generation
- mask parameter parsing/writing (`density`, `feather`)
- relative mask-rectangle hardening (flag bit 0)
- effective `-2` / real `-3` mask semantics hardening
- isolated `clbl` group composition
- vector masks (`vmsk` / `vsms`) parse/write
- zlib-backed ZIP channel/composite read-write without exposing heap allocation through the public API
- adaptive Bezier flattening helpers integrated into the live compositor path
- live `lmgm` / `vmgm` final-crossfade semantics in the compositor
- `test_v17` coverage for:
  - `lmgm` clipping semantics
  - `vmgm` clipping semantics
  - Bezier vector-mask composition
- `lsct` section divider parse/write for folder/group layers
- hierarchical grouped composition for flat PSD layer lists with implicit folder spans
- `tsly` parse/write and conservative grouped transparency-shape semantics
- `test_v18` coverage for pass-through groups, grouped multiply/opacity, nested groups, and `lsct`/`tsly` roundtrip
- finer grouped compositor semantics for `knko`, `infx`, and `tsly`
- clipping-mask propagation across pass-through group wrappers
- classic `lrFX` parse/write plumbing with live solid-fill (`sofi`) composition
- descriptor-summary parser for common Descriptor structure value classes
- preservation-first `TySh` parse/write/roundtrip
- preservation-first `lfx2` parse/write/roundtrip
- preservation-first Smart Object tagged-layer parse/write/roundtrip for `SoLd` / `SoLE`
- `test_v20` coverage for advanced-tag roundtrip, live `lrFX` solid fill, and wrapper-clipping behavior
- deeper classic `lrFX` fidelity plumbing:
  - `cmnS`
  - `dsdw`
  - `isdw`
  - `oglw`
  - `iglw`
  - `bevl`
  - `sofi`
- deeper Descriptor summary coverage for:
  - nested objects
  - lists
  - references
  - unit floats
  - enums
  - raw-data lengths
  - 64-bit comps
- preservation-first `Txt2` parse/write/roundtrip
- deeper Smart Object `plLd` parse/write with typed fields for unique-id, paging, transforms, and warp descriptor summary
- `test_v21` coverage for broader `lrFX`, deeper Descriptor parsing, `Txt2`, and `plLd`

## Verified now

- `make clean && make test` passes
- `tests/test_v12` passes for clipping + merged alpha
- `tests/test_v13` passes for user mask roundtrip + composition
- `tests/test_v14` passes for:
  - shifted mask rectangles
  - density / feather roundtrip
  - effective vs real mask selection
  - `clbl` isolated-group behavior
- `tests/test_v15` passes for:
  - authored vector-mask metadata roundtrip
  - ZIP composite decode
  - ZIP layer-channel decode
  - ZIP+prediction roundtrip
- `tests/test_v16` passes for:
  - adaptive Bezier flattening helpers
  - global-mask tag parsing/writing helpers
  - ZIP diagnostics helpers
- `tests/test_v17` passes for:
  - raster global-mask final-crossfade behavior
  - vector global-mask final-crossfade behavior
  - Bezier-flattened vector-mask composition
- `tests/test_v18` passes for:
  - pass-through group behavior
  - grouped multiply/opacity wrapping
  - nested folder recursion
  - `lsct` / `tsly` roundtrip
- `tests/test_v20` passes for:
  - live `lrFX` solid-fill composition
  - clipping through pass-through wrappers
  - advanced-tag roundtrip for `TySh`, `lfx2`, `SoLd`, `knko`, `infx`, and `tsly`
- `tests/test_v21` passes for:
  - classic `lrFX` field roundtrip for `cmnS`, `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, and `sofi`
  - deeper Descriptor summaries across lists, references, nested objects, unit floats, raw data, enums, and `comp`
  - `Txt2` preservation-first roundtrip
  - Smart Object `plLd` typed-field roundtrip, including transform and warp descriptor summary
- `tests/test_v22` passes for:
  - live drop shadow raster output
  - live outer glow raster output
  - live inner shadow raster output
  - live inner glow raster output
  - live bevel-lite raster output
- `tests/test_v23` passes for:
  - multi-row 2D drop-shadow falloff
  - diagonal outer-glow reach
  - stronger edge-biased inner glow on multi-row content
  - multi-row bevel-lite corner shading
- `tests/corpus_runner` regenerates corpus fixtures successfully, including new v1.8 cases

## Deferred

- exact Photoshop feather kernel / blur semantics
- exact Photoshop pixel-for-pixel classic `lrFX` rendering; `v2.3` keeps a deterministic fixed-point approximation path
- deeper Photoshop-exact knockout semantics and broader `infx`/effects-aware advanced blending
- PSB / 16+ bpc / full-fidelity modern descriptors and non-raster families
- broader interoperability corpus from third-party PSDs
