# ADVANCED NOTES

This tree layers a conservative-but-deeper `v2.1` fidelity pass on top of the integrated `v2.0` tree.

## What is live now

- grouped compositor refinements for:
  - `knko`
  - `infx`
  - `tsly`
  - clipping-mask propagation across pass-through wrappers
- classic `lrFX` parsing/writing with structured coverage for:
  - `cmnS`
  - `dsdw`
  - `isdw`
  - `oglw`
  - `iglw`
  - `bevl`
  - `sofi`
- live compositor application of classic solid-fill overlay (`sofi`)
- preservation-first parse/write/roundtrip for:
  - `TySh`
  - `Txt2`
  - `lfx2`
  - `SoLd` / `SoLE`
  - `plLd`
- descriptor-summary parsing for deeper descriptor value classes, including:
  - `bool`
  - `long`
  - `doub`
  - `UntF`
  - `TEXT`
  - `enum`
  - `comp`
  - lists
  - references
  - nested objects (summary/skip-safe mode)

## What remains conservative

- `lrFX` is not a full Photoshop clone yet.
  - structured parse/write now covers `cmnS`, `dsdw`, `isdw`, `oglw`, `iglw`, `bevl`, and `sofi`
  - live rendering is still intentionally narrower than the file-level coverage and currently centers on `sofi`
- `TySh`, `Txt2`, `lfx2`, and Smart Object tags are handled in a preservation-first way.
  - they roundtrip and expose deeper parsed summaries
  - `plLd` also exposes typed fields for unique ID, paging, transforms, and warp-descriptor summary
  - they still do not attempt to fully interpret every Descriptor-structure branch
- grouped knockout/interior/transparency-shape semantics are stronger than `v1.8`, but still intentionally conservative compared with Photoshop's exact renderer

## Regression entry points

- `tests/test_v20.c`
- `tests/test_v21.c`
- `examples/advanced_roundtrip.c`
