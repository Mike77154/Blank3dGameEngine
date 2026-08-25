# ZRAGF Protocol89 sanitation report

## Target contract

The project was sanitized to the Protocol89 source contract:

- C89/C90 compilation mode.
- Fixed/integer arithmetic only in project-owned C/H sources.
- No project-owned `malloc`, `calloc`, `realloc`, `free`, or heap-backed runtime path.
- No `float`, `double`, `long long`, `int64_t`, `uint64_t`, `intptr_t`, or `uintptr_t` in project-owned C/H sources.
- No `<stdint.h>` or `snprintf` dependency in project-owned C/H sources.
- Tests, fuzzers, corpora, benchmarks, tools, examples, and historical documentation preserved rather than deleted.

## Preservation

- Original files listed in `PROTOCOL89_PRESERVATION_MANIFEST.txt`: **297**
- Missing original files after sanitation: **0**
- Source/infrastructure preservation and forbidden-token checks are automated by `tools/protocol89_audit.py` and wired into CTest as `zragf_test_protocol89_source_audit`.

## Memory model

The default internal allocator was replaced by `zragf_protocol89_mem.c/.h`, a bounded static arena. The default compile-time capacity is **64 MiB** (`ZRAGF_P89_STATIC_BYTES`) and can be overridden at compile time without changing the public codec API. Caller-supplied workspace/custom allocator paths remain available.

Tests, examples, and benchmarks use `protocol89_hostmem.h`, which provides reusable static banks instead of CRT heap allocation.

External comparison/oracle libraries (for example zlib/libdeflate/miniz where enabled) are third-party code and are outside the project-owned no-heap source contract; ZRAGF itself does not call their allocators directly as part of its own storage policy.

## Fixed-point benchmark support

`protocol89_fixed.h` replaces binary floating-point benchmark calculations with integer fixed-scale helpers (x1000 where ratios/rates need fractional precision). Benchmarks were retained and sanitized rather than removed.

## Byte-for-byte golden master

`tests/protocol89_byte_emitter.c` emits 36 deterministic compression records spanning:

- native/raw/zlib/gzip formats,
- levels 1, 5, and 9,
- three deterministic payload families.

Original and sanitized outputs are exactly identical:

- Bytes: **122165**
- SHA-256: `13e5b87531e593d66dfb00db14b85ce0dc25a46990c7c91256b9dc5b5eaa0107`
- `cmp`: exact match

The original golden output is preserved at `tests/golden/protocol89_original_outputs.bin`; `tests/validate_protocol89_byte_exact.py` checks byte identity and the expected SHA-256. CTest runs this as `zragf_test_protocol89_byte_exact`.

Historical Phase 3 emitters were also compared original vs sanitized and matched exactly:

- dynamic: 23 bytes, SHA-256 `b4050d5ba04551285c69fed6586262a3d2486a81e280c193a956bd22c307a131`
- fixed: 279 bytes, SHA-256 `02fa4c1018d01746438abee71aab3879b4a9bf5d7662b7115423f3ea1bfb9b3c`
- stored: 8197 bytes, SHA-256 `14895273b12a3eaa878374a4f8c7602579404668ffae1d5e529a35c6220542ea`

## Validation matrix

Final validation completed successfully:

- Original baseline before sanitation: **65/65 tests passed**.
- Sanitized final CTest suite: **72/72 tests passed**.
- Protocol89 byte-exact test: passed.
- Protocol89 source audit: passed.
- Build with compiler warnings promoted to errors (`-Werror`): passed.
- Clang/libFuzzer smoke, inflate: **200 runs**, no crash.
- Clang/libFuzzer smoke, deflate: **200 runs**, no crash.
- Public/locked corpus tests through phases 67-70: passed.

## Re-run checks

Typical verification from a configured build tree:

```sh
cmake --build .
ctest --output-on-failure
python3 ../tools/protocol89_audit.py
```

For the standalone golden comparison, build/run `zragf_protocol89_byte_emitter` and validate its output with `tests/validate_protocol89_byte_exact.py`.

## Result

The sanitation changes the storage and benchmark-support infrastructure, but the tested encoded byte streams remain byte-for-byte identical to the original library. No original fuzz/test/corpus/tool/documentation file was removed.
