# imgcc0 0.6.0 strict protocol audit

The default imgcc0 image-decoder build is ISO C89 and caller-owned. It contains the replacement PNG/APNG, DDS, BMP, PCX and PSD vendors plus the repaired Protocol89 ZRAGF backend. TIFF and PSD zlib-shaped calls are resolved to ZRAGF by `compat/zlib.h`; PNG/APNG use the explicit imgcc0 ZRAGF bridge.

## Frozen rules

- ISO C89 (`-std=c89 -pedantic-errors`).
- No `malloc`, `calloc`, `realloc` or `free` calls in the active strict source set.
- No `float` or `double` in the active strict source set.
- No explicit 64-bit integer types (`long long`, `int64_t`, `uint64_t`, aliases matched by the auditor).
- Caller-owned output, scratch and file buffers.
- No heap ownership in `imgcc0_image`; reset only clears references/metadata.
- No `libm` and no system `-lz` in the top-level build.

## Active codec set

JPEG, TGA, GIF, QOI, WebP/animated WebP, TIFF, PNG/APNG, DDS, BMP, PCX, PSD89 and Protocol89 ZRAGF are compiled in the default library.

## Current automated gate

`make protocol-audit` scans 166 active C/header files. The final 0.6.0 run reports zero dynamic-allocation calls, zero floating types, zero explicit 64-bit integer types and zero C99 line comments.

`make zragf-audit` passes the ZRAGF Protocol89 source audit (179 C/H files; 297 preserved original files tracked by that tool).

`make zragf-image-compat` runs the dedicated exact-fill/EOB + level-0 zlib compatibility test and the complete TIFF vendor suite through ZRAGF. Both pass.

`make regression` passes 13/13 historical decoded frame outputs byte-for-byte. `make adapter-regression` passes 9/9 PNG/APNG/DDS/BMP/PCX canonical outputs and 24/24 PSD canonical RGBA outputs byte-for-byte.

`make vendor-verify` keeps the original supplied ZRAGF manifest and a repaired manifest. It requires that the only intentional ZRAGF source delta from the supplied package be `zragf_stream.c`; all other replacement vendors remain byte-identical to their supplied packages.

The final archive also checks unresolved symbols in `build/libimgcc0.a`; no system `malloc/calloc/realloc/free` or zlib `inflate/deflate/compress/uncompress` symbols remain unresolved.
