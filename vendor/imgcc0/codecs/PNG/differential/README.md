# Differential testing against libpng — sanitized profile

The original libpng differential utility and curated corpus are restored.
It decodes each static PNG with libpng and with this codec and compares headers,
selected metadata, accept/reject behavior, and final RGBA8 pixels. Adam7 is also
checked through the incremental/poll-driven path.

The project-owned side of the harness follows the same protocol as the codec:
C89, 32-bit counters, Q16.16 metadata, and `png_mem89` for its buffers. The old
floating `gAMA` comparison was replaced with `png_get_gAMA_fixed()` plus Q16.16
tolerance logic. The file loader uses a bounded 16 MiB arena buffer and 32-bit
counters instead of `ftell`/LP64-sensitive storage.

libpng is an external reference dependency. Its callback typedefs are dictated
by libpng's ABI; read sizes are checked against the 32-bit protocol boundary
before being copied into project counters. Where supported, libpng itself is
created with `png_create_read_struct_2()` and its allocations are redirected to
`png_mem89` through user-memory callbacks.

## Run

```sh
./differential/run_local.sh
```

The corpus list is `differential/corpus_static_pngs.txt`.
