# Fuzzing — strict C89 harnesses

The original repository carried corpora and fuzz orchestration but referenced several harness source files that were absent from the archive. This edition restores working harnesses that obey the same C89/no-wide-size protocol as the library.

## Included harnesses

- `tests/fuzz_c89_file.c`: one corpus file per process.
- `tests/fuzz_c89_stdin.c`: stdin/AFL-compatible entry point.
- `tests/fuzz_c89_common.h`: parse/decode/payload exercise logic with static input/RGBA banks.
- parse/decode/payload mode binaries are produced by the Makefile from the same C89 sources.

The file and RGBA capacities are compile-time constants and can be overridden at build time.

## Commands

```sh
make fuzz-c89
make fuzz-c89-modes
make seedcheck-stdin
make seedcheck-stdin-modes
make ci-fuzz-smoke
```

`make fuzz-afl` builds the stdin harness with the configured AFL compiler when available.

A direct libFuzzer ABI adapter is intentionally not compiled in strict mode because its required buffer-length ABI uses the platform native size type, which can be wider than the library's 32-bit contract. The corpus, triage and minimization support is retained around the C89 file/stdin/AFL harnesses instead.
