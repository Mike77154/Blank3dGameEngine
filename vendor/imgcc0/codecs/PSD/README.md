# psd89 v2.3

`psd89` is a deterministic PSD v1 (`.PSD`) library in ANSI C / C89 with a static-only memory policy.

This `v2.3` tree keeps the integrated compositor path from `v2.2`, then pushes **classic live-effect fidelity** harder while preserving the deeper file-fidelity plumbing from `v2.1`:

- **classic `lrFX` is more alive in the compositor**
  - field-level parse/write still covers `cmnS`, `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, and `sofi`
  - live compositor support now renders conservative raster versions of `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, and `sofi`
- **descriptor-heavy families stay deep**
  - richer Descriptor summaries for nested objects, lists, references, unit floats, raw-data lengths, enums, and 64-bit comps
  - deeper typed support for `TySh`, `lfx2`, Smart Object tags (`SoLd`, `SoLE`, `plLd`), and `Txt2`
- **new regression coverage**
  - `test_v22` for live classic-effect raster output
  - `examples/fidelity_roundtrip` for a real PSD authored with multiple live classic effects

## v2.3 highlights

- **broader classic `lrFX` fidelity in the live compositor**
  - drop shadow / outer glow now sample a conservative **2D neighborhood kernel** instead of only the current scanline
  - inner shadow / inner glow now use a more serious **choke-like inner-edge ramp** for multi-row content
  - bevel-lite now uses a conservative **2D edge gradient** on multi-row layers
  - the classic stored `intensity` field is now used as a **spread/choke-style shaping control** for larger kernels
- **new example + regression coverage**
  - `examples/fidelity_roundtrip_v23` writes `examples/example_fidelity_roundtrip_v23.psd`
  - `test_v23` validates 2D falloff for shadow/glow and corner-biased bevel output

## v2.2 highlights

- **live classic-effect compositor path**
  - conservative raster rendering for `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, and `sofi`
  - effect rendering stays static-only and scanline-friendly
  - no public heap allocation added to the API
- **new example + regression coverage**
  - `examples/fidelity_roundtrip` writes `examples/example_fidelity_roundtrip.psd`
  - `test_v22` validates drop shadow, outer glow, inner shadow, inner glow, and bevel-lite output

## v2.1 highlights

- **full-tree `v1.6` integration**
  - `lmgm` / `vmgm` are now live in the compositor
  - adaptive Bezier flattening is now used by the compositor for vector masks
- **global-mask final-crossfade semantics**
  - raster user masks can act as a final crossfade when `lmgm` is enabled
  - vector masks can act as a final crossfade when `vmgm` is enabled
  - fallback behavior for older files is inferred conservatively from stored link/relative flags
- **better vector-mask fidelity**
  - the compositor no longer evaluates vector masks from anchors alone
  - closed Bezier paths are flattened into fixed-point segments and sampled with even-odd fill
- **new regression coverage**
  - `test_v17`
  - new corpus fixtures for:
    - `lmgm` clipping semantics
    - `vmgm` clipping semantics
    - Bezier vector-mask composition

- **real groups / folders**
  - parse/write of `lsct` section divider tagged blocks
  - reconstruction of implicit folder spans from the flat layer list
  - support for open/closed folder markers and bounding section dividers
- **hierarchical compositor path**
  - grouped spans are composited recursively without heap allocation
  - pass-through folders preserve direct child compositing when the wrapper is neutral
  - isolated folder wrappers apply group blend mode and group opacity as a single composite step
- **advanced blending metadata hardening**
  - parse/write of `tsly` (`Transparency shapes layer`)
  - conservative raster semantics for grouped transparency-shape handling
- **new regression coverage**
  - `test_v18`
  - corpus fixtures for pass-through, grouped multiply, and nested grouped composition

## Supported subset

### Read

- classic PSD (`8BPS`, version `1`) only
- Gray / RGB, 8 bpc only
- composite image decode
- layer records and per-layer channel offsets
- on-demand decode of any present layer channel
- parse clipping flags and tagged booleans:
  - `clbl`
  - `infx`
  - `knko`
  - `lmgm`
  - `vmgm`
- parse user mask metadata, including:
  - relative-mask flag handling
  - density / feather parameters
  - effective `-2` vs real `-3` selection data
- parse vector masks (`vmsk` / `vsms`):
  - closed/open subpaths
  - linked/unlinked knots
  - initial fill rule marker
- preserve unknown image resources and tagged blocks as passthrough refs

- classic effects (`lrFX`) parse/write for:
  - `cmnS`
  - `dsdw`
  - `isdw`
  - `oglw`
  - `iglw`
  - `bevl`
  - `sofi`
- typed parse of advanced tagged blocks:
  - `TySh`
  - `Txt2`
  - `lfx2`
  - `SoLd` / `SoLE` / `plLd`
- richer descriptor summaries for common and nested Descriptor payloads (`bool`, `long`, `comp`, `doub`, `UntF`, `TEXT`, `enum`, lists, references, nested objects, raw-data blobs)

### Write

- classic PSD, Gray / RGB, 8 bpc
- composite image write
- raster layers
- RAW, RLE, ZIP, ZIP+prediction
- authored clipping flags / opacity / name / blend mode key
- authored booleans:
  - `clbl`
  - `infx`
  - `knko`
  - `lmgm`
  - `vmgm`

- authored advanced tagged blocks:
  - `lrFX` (`cmnS`, `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, `sofi`)
  - `TySh`
  - `Txt2`
  - `lfx2`
  - `SoLd` / `SoLE` / `plLd`
- authored user masks with:
  - relative rectangles
  - density / feather
  - optional real-user-mask metadata block
- authored vector masks (`vmsk` / `vsms`)
- merged alpha write for authored RGB/Gray composites
- passthrough re-emission of unknown resources and tagged blocks

### Compose

- source-over compositor in fixed-point `Q16.16`
- compose from authored memory planes or parsed PSD channel data
- live conservative raster rendering for classic `lrFX` families:
  - `dsdw`
  - `isdw`
  - `oglw`
  - `iglw`
  - `bevl`
  - `sofi`
- clipping-base masking for successive `clipping = 1` layers
- isolated clipped-group semantics when `clbl` is enabled on the base layer
- user mask modulation with:
  - density
  - deterministic rect-edge feather ramp
  - effective `-2` vs real `-3` selection
- vector mask modulation with:
  - adaptive cubic Bezier flattening
  - even-odd fill over flattened segments
  - vector density / feather modulation when present
- final-crossfade mask semantics for:
  - `lmgm`
  - `vmgm`

## Conservative choices in v2.2

A few behaviors are intentionally conservative rather than full-Photoshop clones:

- user feather is applied as a deterministic **resolved-rect edge falloff**, not as Photoshop's exact blur kernel
- vector feather is applied as a deterministic **vector-bounds falloff**, not as Photoshop's exact blur kernel
- vector masks are rasterized from an **adaptive flattened approximation** of the Bezier path, not from analytical curve intersections
- fallback behavior for old files that lack `lmgm` / `vmgm` is inferred conservatively from the available stored flags
- ZIP implementation uses zlib allocator hooks and does not expose heap allocation through the `psd89` API
- ZIP+prediction is implemented for the supported **8 bpc Gray/RGB subset**
- classic live `lrFX` rendering in `v2.3` uses deterministic **fixed-point raster approximations**; shadows and glows now use a conservative multi-row kernel, but the renderer still is not Photoshop-exact

## Intentional limits still out of scope

- PSB
- 16/32 bpc
- Photoshop-exact rendering for editable text, Smart Objects, object effects, and other modern non-raster families
- exact Photoshop pixel-for-pixel classic-`lrFX` rendering; `v2.3` keeps a deterministic approximation path
- exact Photoshop knockout / layer-style interactions beyond the conservative raster path in `v1.8`
- exact Photoshop feather semantics

## Build

```sh
make
make test
```

Build outputs:

- `libpsd89.a`
- `examples/file_roundtrip`
- `examples/layer_mask_roundtrip`
- `examples/fidelity_roundtrip`
- `tests/test_v12`
- `tests/test_v13`
- `tests/test_v14`
- `tests/test_v15`
- `tests/test_v16`
- `tests/test_v17`
- `tests/test_v18`
- `tests/test_v21`
- `tests/test_v22`
- `tests/test_v23`
- `tests/corpus_runner`

## Tests and corpus

`make test` runs:

- `tests/test_v12` for clipping + merged alpha
- `tests/test_v13` for user mask composition + roundtrip
- `tests/test_v14` for:
  - shifted relative mask rectangles
  - density / feather parsing + roundtrip
  - effective `-2` vs real `-3` semantics
  - isolated `clbl` composition
- `tests/test_v15` for:
  - authored vector-mask metadata roundtrip
  - ZIP composite decode
  - ZIP layer-channel decode
  - ZIP+prediction roundtrip
- `tests/test_v16` for:
  - adaptive Bezier flattening helpers
  - global-mask tag parsing/writing helpers
  - ZIP diagnostics helpers
- `tests/test_v17` for:
  - `lmgm` clipping semantics in the compositor
  - `vmgm` clipping semantics in the compositor
  - Bezier vector-mask composition in the compositor
- `tests/test_v18` for:
  - pass-through folder composition
  - grouped multiply/opacity wrapping
  - nested folder recursion
  - `lsct` / `tsly` roundtrip parsing and writing
- `tests/test_v21` for:
  - classic `lrFX` family roundtrip beyond `sofi`
  - deeper Descriptor summaries through `TySh`, `lfx2`, and Smart Object tags
  - `Txt2` text-engine-data roundtrip
  - `plLd` placed-layer metadata + warp descriptor roundtrip
- `tests/test_v22` for:
  - live drop shadow raster output
  - live outer glow raster output
  - live inner shadow raster output
  - live inner glow raster output
  - live bevel-lite raster output
- `tests/corpus_runner` to regenerate:
  - `tests/corpus/rgb_user_mask_raw.psd`
  - `tests/corpus/rgb_user_mask_rle.psd`
  - `tests/corpus/rgb_mask_shift_density_rle.psd`
  - `tests/corpus/rgb_real_user_mask_rle.psd`
  - `tests/corpus/rgb_clbl_group_rle.psd`
  - `tests/corpus/rgb_clbl_individual_rle.psd`
  - `tests/corpus/rgb_vector_mask_zip.psd`
  - `tests/corpus/rgb_vector_mask_zip_pred.psd`
  - `tests/corpus/rgb_lmgm_clip_rle.psd`
  - `tests/corpus/rgb_vmgm_clip_rle.psd`
  - `tests/corpus/rgb_vector_bezier_rle.psd`
  - `tests/corpus/rgb_group_pass_rle.psd`
  - `tests/corpus/rgb_group_mul_rle.psd`
  - `tests/corpus/rgb_nested_group_mul_rle.psd`

## Tree

```text
psd89/
├── Makefile
├── README.md
├── STATUS.md
├── ROADMAP_V2.md
├── ADVANCED_NOTES.md
├── FIDELITY_NOTES.md
├── BUILD_LOG.txt
├── include/
│   └── psd89/
│       ├── psd89.h
│       └── psd89_stdio.h
├── src/
│   ├── psd89_advanced.c
│   ├── psd89_compose.c
│   ├── psd89_core.c
│   ├── psd89_internal.h
│   ├── psd89_mask_global_tags.c
│   ├── psd89_read.c
│   ├── psd89_stdio.c
│   ├── psd89_util.c
│   ├── psd89_vector_bezier.c
│   ├── psd89_write.c
│   ├── psd89_zip.c
│   └── psd89_zip_diag.c
├── examples/
│   ├── advanced_roundtrip.c
│   ├── file_roundtrip.c
│   ├── fidelity_roundtrip.c
│   ├── group_roundtrip.c
│   └── layer_mask_roundtrip.c
└── tests/
    ├── corpus/
    ├── corpus_runner.c
    ├── test_v12.c
    ├── test_v13.c
    ├── test_v14.c
    ├── test_v15.c
    ├── test_v16.c
    ├── test_v17.c
    ├── test_v18.c
    ├── test_v20.c
    ├── test_v21.c
    └── test_v22.c
```

