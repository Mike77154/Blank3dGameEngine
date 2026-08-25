# Fuzzing — C89/static-memory profile

All original fuzz targets and all original seed corpora are restored.

Targets cover full PNG decode, incremental decode, APNG decode, PNG/APNG encode,
unknown chunks, public-format conversion, Adam7, and metadata.

## Protocol rules used by the fuzz harnesses

- target sources compile as C89
- no project-owned `malloc`/`calloc`/`realloc`/`free`
- no floating-point types or literals
- no explicit 64-bit integer types
- fuzz-owned temporary buffers come from `png_mem89`
- every fuzz entry resets the static arena before starting
- the fuzz input length is a 32-bit `png_u32` and is capped at 8 MiB
- zlib allocations are also routed through `png_mem89`

The external libFuzzer/AFL/sanitizer runtime is host tooling and may use its own
implementation details; no such allocation API is exposed or used by the project C sources.

## Local libFuzzer build

```sh
SANITIZER=address MODE=libfuzzer ./fuzz/build_local.sh
SANITIZER=undefined MODE=libfuzzer ./fuzz/build_local.sh
```

Example:

```sh
ASAN_OPTIONS=detect_leaks=0 ./fuzz/out/local/png_fuzz_decode_full \
  -runs=1000 fuzz/corpus/png_fuzz_decode_full
```

## Standalone deterministic corpus gate

```sh
./fuzz/run_corpus_standalone.sh
```

This builds every target with the static stdin driver and feeds every restored
seed through its matching target. It is useful on machines without libFuzzer.

## AFL++

```sh
MODE=afl CC=afl-clang-fast SANITIZER=address ./fuzz/build_local.sh
```

The AFL standalone driver has a fixed 8 MiB input buffer and performs no dynamic allocation.

## OSS-Fuzz / ClusterFuzzLite

`fuzz/build.sh` keeps the original seed-corpus packaging behavior, but adds the
C89 protocol compiler flags and builds the sanitized codec sources.
