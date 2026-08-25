# BMP89 — C89/static-workspace sanitation

This package is the sanitized BMP codec core.

## Contract

- ISO C89 source only.
- Integer/fixed-point domain only; no floating-point arithmetic.
- No dynamic-memory ownership in the codec.
- No native 64-bit integer types or arithmetic.
- Public sizes and capacities are 32-bit unsigned values.
- Encoder and payload-copy APIs write into caller-provided buffers/workspaces.
- Decoder writes into caller-provided RGBA storage.

The codec itself has no fractional algorithm that needs floating-point. Exact channel scaling is implemented with checked integer arithmetic. A signed 16.16 scalar type (`bmp_fx16_16`) is provided for fixed-point integration where a fractional scalar is useful.

## Build

```sh
make
make test
```

The default build uses strict C89 plus pedantic diagnostics and warnings-as-errors.

## Important API migration

Convenience functions that formerly returned dynamically owned buffers were intentionally removed. Use the `*_into` and workspace-size APIs instead:

- `bmp_encode_rgba32_workspace_size`
- `bmp_encode_indexed_workspace_size_bound`
- `bmp_encode_rgba32_into`
- `bmp_encode_indexed_into`
- `bmp_decode_to_rgba32`
- `bmp_copy_embedded_payload_into`

This is a source/API compatibility break for callers that used ownership-returning convenience calls; encoded and decoded content is the compatibility target.

## Verification

See `verification/BYTE_IDENTITY_REPORT.md`. The sanitation was checked against the original codec by generating/decoding representative BMPs and comparing outputs byte for byte.
