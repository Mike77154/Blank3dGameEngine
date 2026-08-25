# BMP codec — C89/static-workspace edition

This directory contains the BMP parser, decoder, encoder, metadata, reports, filters and render callback layer.

Supported families include core/info/V4/V5 style headers, indexed 1/2/4/8-bit data, RGB555/RGB565/BGR24/32-bit variants, bitfields/alpha bitfields, RLE4/RLE8, top-down BMPs and embedded BI_JPEG/BI_PNG payload inspection/extraction.

## Memory contract

The codec does not own dynamic storage. Parsing references caller-owned input bytes. Decoding writes RGBA into caller-owned storage. Encoding writes both temporary pixel data and the final BMP into caller-provided workspace/output banks.

Useful entry points:

- `bmp_parse_memory()`
- `bmp_calc_rgba32_buffer_size()` + `bmp_decode_to_rgba32()`
- `bmp_encode_rgba32_workspace_size()` + `bmp_encode_rgba32_into()`
- `bmp_encode_indexed_workspace_size_bound()` + `bmp_encode_indexed_into()`
- `bmp_get_embedded_payload()` / `bmp_copy_embedded_payload_into()`
- diagnostics, filters and callback rendering helpers

All codec sizes/capacities are 32-bit. Checked integer helpers reject overflow. Fractional integration can use the provided signed 16.16 scalar type; the BMP algorithms themselves do not require floating-point arithmetic.
