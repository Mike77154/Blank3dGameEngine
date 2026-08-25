# Fuzzing workflow (round 4)

This round adds practical workflow helpers on top of the round 3 harnesses:

- corpus minimization for libFuzzer (`-merge=1`)
- corpus minimization for AFL++ (`afl-cmin`) and optional per-file trimming (`afl-tmin`)
- crash reproduction helpers
- crash triage reports with optional minimization
- a persistent AFL-style harness source with a regular fallback path for non-AFL builds

## Quick commands

```sh
make fuzz-libfuzzer
make seedcheck-libfuzzer
make corpus-min-libfuzzer
make fuzz-persistent
make persistent-check
```

If AFL++ is installed:

```sh
make fuzz-afl
make fuzz-afl-persistent
make corpus-min-afl
make corpus-tmin-afl
```

## Crash triage

Put crash inputs in `tests/fuzz_crashes/`, then run one of:

```sh
make crash-triage-libfuzzer
make crash-triage-stdin
```

Reports are written to `tests/fuzz_triage_libfuzzer/` or `tests/fuzz_triage_stdin/`, depending on the target.

## Reproduce one input

```sh
sh ./scripts/repro_crash.sh libfuzzer ./tests/fuzz_bmp_libfuzzer path/to/input
sh ./scripts/repro_crash.sh file ./tests/fuzz_bmp_stdin path/to/input
```


## Round 5: CI + split harnesses

This round adds pathway-focused fuzz entry points and GitHub Actions smoke coverage:

- `tests/fuzz_libfuzzer_parse.c`
- `tests/fuzz_libfuzzer_decode.c`
- `tests/fuzz_libfuzzer_payload.c`
- `tests/fuzz_stdin_parse.c`
- `tests/fuzz_stdin_decode.c`
- `tests/fuzz_stdin_payload.c`
- `.github/workflows/ci.yml`

Useful commands:

```sh
make fuzz-libfuzzer-modes
make fuzz-stdin-modes
make seedcheck-libfuzzer-modes FUZZ_RUNS=64
make seedcheck-stdin-modes
make ci-fuzz-smoke FUZZ_RUNS=64
```

The CI workflow builds with both GCC and Clang, then runs a Clang/libFuzzer smoke pass on the shared corpus.

## Round 6: sanitizers, per-target corpus minimization, and CI artifacts

This round tightens the CI loop around the split harnesses:

- sanitizer smoke coverage for `tests/test_smoke`
- per-target smoke jobs for `parse`, `decode`, and `payload`
- per-target libFuzzer corpus minimization targets
- artifact bundling helper: `scripts/bundle_ci_artifacts.sh`
- CI uploads logs + source corpus + minimized corpus for each target job

Useful commands:

```sh
make test-sanitize CFLAGS='-std=c99 -O1 -g -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -fsanitize=address,undefined'
make seedcheck-libfuzzer-parse FUZZ_RUNS=64 FUZZ_SANITIZERS=address,undefined
make seedcheck-stdin-decode
make corpus-min-libfuzzer-payload FUZZ_SANITIZERS=address,undefined
make bundle-ci-parse
```


## Round 7: crash bucketing + HTML/JSON artifact indexes

This round improves the post-fuzz workflow in two directions:

- finer crash bucketing in `scripts/triage_crashes.py`
  - sanitizer kind
  - normalized signature
  - top relevant frames
- richer CI artifact bundles with machine-readable and human-readable indexes:
  - `manifest.json`
  - `index.json`
  - `index.html`
  - `summary.md`

Useful commands:

```sh
make crash-buckets-libfuzzer
make crash-buckets-stdin
make test-round7
make bundle-ci-parse
```

The bundle helper can now include extra directories such as triage outputs, and the GitHub Actions workflow appends `summary.md` to `$GITHUB_STEP_SUMMARY`.
