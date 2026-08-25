# PCX security notes

This build adds process-wide decode limits intended to make hostile PCX inputs cheaper to reject. By default the library caps input size, dimensions, total pixels, decoded output bytes and per-scanline working set. Set a field to `0` to disable that limit, or call `pcx_set_global_decode_limits(NULL)` to restore defaults.

Thread-safety note: the global limits object is process-wide and is not synchronized internally. Set it during process startup or protect changes externally if multiple threads may decode concurrently.

Operational guidance:

- keep fuzzing enabled in CI
- keep strict mode for untrusted inputs when possible
- lower the default limits for sandboxed or server-side deployments
- treat symbolic error strings and human descriptions as logging aids, not stable ABI
