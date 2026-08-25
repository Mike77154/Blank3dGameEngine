# imgcc0 0.5.0 PSD89 integration report

## Vendor replacement

The supplied `PSD_C89_FIXED_STATIC_BYTEEXACT` package is vendored under `codecs/PSD/` without modifying vendor contents. `vendor_manifests/PSD.sha256` records every supplied file. A fresh extraction of the uploaded ZIP compares byte-for-byte with the vendored tree.

## Protocol characteristics

The PSD89 C/header sources contain no calls to `malloc`, `calloc`, `realloc`, or `free`; no native `float`/`double`; and no native `long long`, `int64_t`, or `uint64_t`. The library uses 32-bit integer/fixed-point operations. ZIP/ZIP+prediction uses zlib with a fixed internal arena allocator.

The supplied library passes its exact strict build command:

```sh
make -B CFLAGS='-std=c89 -Wall -Wextra -pedantic -Werror -Iinclude' all
make test CFLAGS='-std=c89 -Wall -Wextra -pedantic -Werror -Iinclude'
```

All `test_v12` through `test_v23` targets and the corpus runner pass. The supplied verification manifests contain 28 baseline outputs and 28 sanitized outputs with identical SHA-256 entries.

## imgcc0 adapter

The facade now calls `psd89_read()` and `psd89_decode_composite_u8()` directly. `psd89_doc` and decoded planar channels come from `imgcc0_open_options.temp_buffer`; the persistent RGBA8 frame remains in `output_buffer`. No ownership is transferred to the codec or facade.

Supported facade path follows the PSD89 read subset: classic PSD version 1, 8 bpc, Gray or RGB composite image, with RAW/RLE/ZIP/ZIP+prediction. Gray is expanded to RGB. When strict mode is enabled, merged alpha is used only when PSD89 reports the merged-alpha flag; otherwise alpha is 255. Extra non-alpha channels are decoded into a reusable discard plane.

The PSD document structure is approximately 1.61 MiB on the tested target; RGB composite decoding additionally needs approximately 3 x width x height temporary bytes, plus one additional plane when merged alpha is present and one reusable discard plane when extra channels exist. ZIP decoding also uses PSD89's fixed zlib arena on the call stack.

## Validation

- Top-level strict build: PASS with `-std=c89 -pedantic-errors`.
- Active-source protocol audit: PASS, 166 C/header files.
- Unresolved `malloc/calloc/realloc/free` symbols in `libimgcc0.a`: none.
- Vendor integrity: PSD 90/90 files byte-identical to the supplied package.
- Direct PSD89 vs imgcc0 facade: 24/24 supplied PSD files byte-identical in canonical RGBA8 output.
- Existing replacement adapter parity: 9/9 still PASS.
- Existing legacy-route regression: 13/13 frames still PASS.

## Deliberate non-change

The PSD vendor itself was not patched to use ZRAGF. Its ZIP implementation remains exactly as supplied and uses the zlib API with its own fixed arena allocator. This keeps vendor integrity and isolates the PSD integration from a separate compression-backend migration.
