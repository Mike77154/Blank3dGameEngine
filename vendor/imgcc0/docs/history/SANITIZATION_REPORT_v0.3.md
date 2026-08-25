# imgcc0 0.3.0 infrastructure/vendor swap report

## Scope

The strict `imgcc0` facade remains caller-owned/no-heap. This revision removes the old PNG, DDS, BMP, PCX and PSD vendor trees, then vendors the four replacement packages actually supplied (PNG, DDS, BMP and PCX) without modifying their contents. No replacement PSD package was supplied, so PSD remains detected but quarantined.

## Structural changes

- Removed allocator callback types from the public facade API.
- Removed allocator state from `imgcc0_image`.
- Replaced owned frame/pixel allocation with a caller-owned monotonic output arena.
- Replaced transient allocation with a caller-owned scratch arena.
- Changed file loading to use a caller-owned file buffer.
- Replaced image destruction semantics with `imgcc0_image_reset()`; the facade has nothing to release.
- Added storage usage/peak reporting to `imgcc0_image`.
- Added `IMGCC0_ERR_PROTOCOL` for detected codecs whose vendored implementation is not yet safe to enter.
- Moved the old zlib shim and compatibility header to `legacy/unsanitized/` and excluded them from the strict build.
- Default Makefile now uses ISO C89 + pedantic errors and does not link `libm`.

## Default strict build

Protocol-ready routes:

- JPEG
- TGA
- GIF
- QOI
- WebP
- animated WebP
- TIFF
- PNG / APNG
- DDS
- BMP
- PCX

Quarantined route:

- PSD (old vendor removed; replacement not supplied)

## Verification performed

1. `make` passes with `-std=c89 -pedantic-errors`.
2. `make protocol-audit` scans 92 active strict-build C/header files and finds:
   - 0 dynamic-allocation calls;
   - 0 floating types;
   - 0 explicit 64-bit integer types;
   - 0 C99 line comments.
3. The built archive has no unresolved `malloc`, `calloc`, `realloc`, `free`, or common floating math symbols.
4. AddressSanitizer + UndefinedBehaviorSanitizer were run across JPEG, TGA, GIF, QOI, WebP, animated WebP and TIFF samples after the arena rewrite; the final pass completed without sanitizer errors.
5. `make regression` verifies 13 decoded frame files against the existing SHA-256 golden outputs. All 13 are byte-identical.
6. `make vendor-verify` verifies the new vendor directories against manifests made from the supplied packages: PNG 230 files, DDS 80 files, BMP 250 files and PCX 234 files; all are byte-identical. It also verifies that the old PSD vendor is absent.
7. `make adapter-regression` compares the imgcc0 adapter output to direct vendor output for 9 canonical PNG/APNG, DDS, BMP and PCX cases; all are byte-identical.

## Intentional behavior change

PNG/APNG, DDS, BMP and PCX are active again through the supplied replacement vendors. PSD is the only one of this swap set that still returns `IMGCC0_ERR_PROTOCOL`, because the fourth supplied replacement archive is PCX89 rather than PSD and no PSD replacement package is present.
