# BMP C89 Fixed-Point Full Restoration Report

## Goal
Restore the useful development/support surface from the original BMP repository without reintroducing dynamic allocation, native 64-bit integer types, `long`, `size_t`, `float`, or `double` into shipped C/header code.

## Restored support
- `tools/`: diagnostic CLI, compatibility CLI, integer-only benchmark.
- `tests/`: broad smoke test, public API test, contract tests, C89 header test, fuzz corpus, generated/external corpus metadata.
- fuzzing: strict C89 file/stdin harnesses plus AFL-compatible target and parse/decode/payload modes.
- industrial build: Makefile, CMake, presets, pkg-config, Conan/vcpkg metadata, install/export logic, CPack.
- API annotations: visibility, nonnull, access, warn-unused-result, SAL/compiler contracts, pure/const/cold/noinline/deprecation helpers where truthful for caller-owned memory.
- historical/project documentation, governance, policies, release/support scripts, examples and packaging metadata.

## Intentional protocol adaptations
- Allocator/deallocator and allocation-returning public APIs remain removed; callers provide output/workspace buffers.
- Direct libFuzzer ABI entrypoint is not shipped because its required native `size_t` length parameter violates this repository's strict no-native-wide-size protocol on 64-bit hosts. File/stdin/AFL harnesses provide repeatable fuzzing without that ABI exception.
- Obsolete C99/C11/C++ header compile samples were replaced by a strict C89 header sample.
- The mismatched-deallocator negative test was removed because the library no longer owns dynamically allocated buffers.
- Historical docs that describe the pre-sanitation allocator API are preserved under `historical_docs/pre_sanitation/` and are explicitly marked historical.

## Verification performed
- Strict build: `-std=c89 -pedantic-errors` with hard warnings: PASS.
- Protocol audit across shipped C/header sources: PASS.
- Full smoke suite: PASS (`test_smoke: OK`).
- Contract/header/buffer/compiler audits: PASS.
- Fuzz seed corpus: 39/39 runner exits clean; parse/decode/payload harness variants build and run.
- Compatibility matrix: 6/6 expected outcomes matched.
- Shared-library export allowlist and SONAME check: PASS (51 public exports).
- Installed public API smoke: PASS.
- API surface, symbol visibility and portability audits: PASS.
- Packaging metadata audit: PASS after correcting the restored vcpkg port from stale 1.15.0 to 1.16.0.
- CPack smoke: binary TGZ, binary ZIP and source TGZ generated successfully and audited.
- Independent CMake build with tests/tools/C89 fuzz harnesses enabled: 2/2 CTest tests passed.

## Byte identity
The restored FULL package uses the same eight sanitized core `.c` files as the previously differential-tested sanitation build; SHA-256 comparison confirms all eight are byte-identical. The preserved differential records report:
- encoder output: 15/15 byte-identical BMP files;
- parse/metadata/RGBA decode: 60/60 byte-identical records/output;
- diagnostics reports: 27/27 byte-identical.

A fresh encoder differential run against this FULL tree also produced 15/15 byte-identical BMP outputs.

## What was deliberately not resurrected
Only support artifacts whose premise is incompatible with the new memory model were retired/replaced: the dynamic-deallocator negative test and C99/C11/C++ header dialect samples. No BMP decoder/encoder module or supported BMP format was removed.
