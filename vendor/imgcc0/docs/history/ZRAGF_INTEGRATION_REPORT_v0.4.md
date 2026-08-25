# imgcc0 0.4.0 ZRAGF Protocol89 integration

## Scope

- Removed the previous `codecs/ZRAGF` tree.
- Vendored the supplied `ZRAGF_C89_PROTOCOL89` tree without editing vendor files.
- Added the ZRAGF library sources to the default `libimgcc0.a` build.
- Added `src/imgcc0_zragf_bridge.c/.h` outside the vendor tree.
- Routed imgcc0 PNG/APNG one-shot DEFLATE inflation through ZRAGF.
- Left TIFF's existing zlib backend unchanged so the ZRAGF swap remains isolated and regression-testable.

## Vendor integrity

The supplied ZRAGF tree contains 308 files in this package. A recursive path/hash comparison between the extracted upload and `codecs/ZRAGF` reports:

- missing: 0
- extra: 0
- changed: 0

`vendor_manifests/ZRAGF.sha256` and `scripts/verify_vendor_swap.py` keep this check reproducible.

## Protocol checks

The active imgcc0 protocol audit scans 152 active C/header files and reports:

- dynamic-allocation calls: 0
- `float` / `double`: 0
- explicit 64-bit integer types: 0
- C99 `//` comments: 0

ZRAGF's own Protocol89 source auditor also passes: 179 C/H files scanned and 297 original files preserved.

The final `libimgcc0.a` has no unresolved `malloc`, `calloc`, `realloc`, or `free` symbols.

## ZRAGF golden master

The vendored `zragf_protocol89_byte_emitter` was built from the integrated tree and compared against its preserved golden output:

- bytes: 122165
- SHA-256: `13e5b87531e593d66dfb00db14b85ce0dc25a46990c7c91256b9dc5b5eaa0107`
- result: byte-exact PASS

## PNG/APNG backend parity

The ZRAGF bridge handles the exact-output-buffer edge case explicitly: when the caller buffer fills exactly, the bridge gives the inflater a one-byte sink only to finish wrapper/checksum state; success is accepted only when total output does not increase.

Compared against imgcc0 0.3.0's zlib-backed facade:

- PNG RGBA8: byte-identical
- PNG Adam7: byte-identical
- APNG frame 0: byte-identical
- APNG frame 1: byte-identical
- APNG frame 2: byte-identical

The normal adapter regression also passes all 9 PNG/APNG, DDS, BMP and PCX canonical cases.

## Regression

- Existing protocol-ready routes: 13/13 frame outputs byte-identical.
- Replacement adapter parity: 9/9 canonical outputs byte-identical.
- ZRAGF vendor integrity: 308/308 files byte-identical to the supplied package.
