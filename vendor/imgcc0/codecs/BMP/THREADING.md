# Threading

The codec core stores only the configurable global limits object as shared mutable state. Callers that need per-thread limits should prefer the per-call variants.

Reentrant caller-owned operations include:

- `bmp_parse_memory_with_limits()`
- `bmp_decode_to_rgba32_with_limits()`
- `bmp_encode_rgba32_into()`
- `bmp_encode_indexed_into()`
- `bmp_copy_embedded_payload_into()`

Each concurrent call must use separate output/workspace storage unless the caller provides its own synchronization. The codec does not create or own per-call storage.
