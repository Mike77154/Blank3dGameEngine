# imgcc0 0.6.0 — ZRAGF TIFF/PSD decoder cutover

`imgcc0` is a small C89 image-codec facade with format detection, RGBA8 frame output, animation frame metadata, pixel access and surface blitting.

Version 0.6.0 keeps the strict caller-owned facade and replacement codec vendors, repairs two DEFLATE/zlib compatibility defects in Protocol89 ZRAGF, and routes TIFF and PSD zlib-shaped decode calls through ZRAGF. PNG/APNG continue through the explicit ZRAGF bridge. The top-level image-decoder build no longer links the system zlib library.

## Strict protocol

The active facade and the default library build use:

- ISO C89 (`-std=c89 -pedantic-errors`).
- caller-owned output storage;
- caller-owned temporary/scratch storage;
- caller-owned file input storage for `imgcc0_open_file`;
- no dynamic-allocation calls in the strict build;
- no `float` or `double` in the strict build;
- no explicit 64-bit integer types (`long long`, `int64_t`, `uint64_t`, etc.);
- no `libm` dependency.

`imgcc0_image_reset()` only clears metadata and pointers. It never releases storage because the facade never owns the buffers.

## Protocol-ready codecs in the default build

- JPEG
- TGA
- GIF
- QOI
- WebP
- animated WebP
- TIFF
- PNG / APNG (replacement vendor)
- DDS (replacement vendor)
- BMP (replacement vendor)
- PCX (replacement vendor)
- ZRAGF Protocol89 (compiled backend; PNG/APNG, TIFF and PSD DEFLATE/zlib decode paths)
- PSD / PSD89 (replacement vendor; Gray/RGB 8 bpc composite decode to RGBA8)

## Memory model

```text
caller
├── output_buffer
│   ├── imgcc0_frame[]
│   └── RGBA8 frame pixels
├── temp_buffer
│   └── decoder scratch / transient conversion buffers
└── file_buffer
    └── source bytes used by imgcc0_open_file()
```

`imgcc0_open_memory()` does not require `file_buffer` because the source bytes already belong to the caller.

## Minimal example

```c
#include "imgcc0.h"

#define OUT_BYTES  (4U * 1024U * 1024U)
#define TEMP_BYTES (4U * 1024U * 1024U)
#define FILE_BYTES (2U * 1024U * 1024U)

IMGCC0_DECLARE_BUFFER(g_out, OUT_BYTES);
IMGCC0_DECLARE_BUFFER(g_temp, TEMP_BYTES);
IMGCC0_DECLARE_BUFFER(g_file, FILE_BYTES);

int load_image(const char *path)
{
    imgcc0_open_options opt;
    imgcc0_image img;

    imgcc0_open_options_init(&opt);
    imgcc0_image_init(&img);

    opt.output_buffer = IMGCC0_BUFFER_DATA(g_out);
    opt.output_buffer_size = IMGCC0_BUFFER_SIZE(g_out);
    opt.temp_buffer = IMGCC0_BUFFER_DATA(g_temp);
    opt.temp_buffer_size = IMGCC0_BUFFER_SIZE(g_temp);
    opt.file_buffer = IMGCC0_BUFFER_DATA(g_file);
    opt.file_buffer_size = IMGCC0_BUFFER_SIZE(g_file);

    if (imgcc0_open_file(path, &opt, &img) != IMGCC0_OK) {
        return 0;
    }

    /* img.frames[i].pixels is RGBA8 and remains in g_out. */
    imgcc0_image_reset(&img);
    return 1;
}
```

## Build and checks

```sh
make
make protocol-audit
make regression
make vendor-verify
make adapter-regression
make zragf-audit
make zragf-image-compat
```

The legacy-route regression verifies **13 frame outputs byte-for-byte** against SHA-256 digests captured before the vendor swap. `vendor-verify` proves PNG, DDS, BMP, PCX and PSD remain byte-identical to their supplied packages and verifies the repaired ZRAGF tree against its post-fix manifest; it also asserts that `zragf_stream.c` is the only intentional ZRAGF delta from the supplied package. `adapter-regression` compares direct-vendor canonical output with the imgcc0 facade: 9 PNG/APNG, DDS, BMP and PCX cases plus 24 PSD files. `zragf-image-compat` directly tests exact-fill/EOB handling, zlib level-0 stored behavior, and the complete TIFF vendor suite through ZRAGF.

## Active tree

```text
include/imgcc0.h
src/imgcc0.c
demos/demo_cli.c
scripts/audit_imgcc0_protocol.py
scripts/test_strict_samples.py
PROTOCOL_AUDIT.md
Makefile
codecs/
```

TIFF and PSD include zlib-shaped source calls, but the strict build resolves them through `compat/zlib.h` to Protocol89 ZRAGF. The top-level Makefile therefore has no `-lz` dependency. See `ZRAGF_ZLIB_FIX_REPORT.md` for the repaired exact-fill/EOB and level-0 cases and the byte-parity evidence.
