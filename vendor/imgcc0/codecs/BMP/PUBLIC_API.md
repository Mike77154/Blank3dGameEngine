# Public API — strict C89 edition

The installed API is `include/bmp/bmp.h` plus `include/bmp/bmp_export.h`.

## Storage model

The codec never owns dynamically-created storage. Input, output and temporary work memory are supplied by the caller and sized with 32-bit capacities.

Primary entry points:

- `bmp_parse_memory()` / `bmp_parse_memory_with_limits()`
- `bmp_calc_rgba32_buffer_size()`
- `bmp_decode_to_rgba32()` / `bmp_decode_to_rgba32_with_limits()`
- `bmp_encode_rgba32_workspace_size()`
- `bmp_encode_indexed_workspace_size_bound()`
- `bmp_encode_rgba32_into()`
- `bmp_encode_indexed_into()`
- `bmp_get_embedded_payload()` / `bmp_copy_embedded_payload_into()`
- diagnostic/report/filter/render helpers

All public byte counts, offsets, sizes and capacities use `bmp_u32`. Operations that cannot be represented by the 32-bit contract fail instead of wrapping.

See `MIGRATION_C89.md` for the ownership-model migration and `historical_docs/pre_sanitation/` for preserved pre-sanitation API documentation.
