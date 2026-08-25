# ZRAGFLIB roadmap status (phase 69)

## What moved in phase 65

- Kept configurable **global allocator hooks** for one-shot/native paths.
- Kept **per-stream allocator hooks** through `z_stream`-style `zalloc` / `zfree`.
- Added **wrapper-stream workspace helpers**:
  - `zragf_workspace_init()`
  - `zragf_workspace_reset()`
  - `zragf_workspace_used()`
  - `zragf_stream_set_workspace()`
  - `zragf_deflate_workspace_bound_z()`
  - `zragf_inflate_workspace_bound_z()`
- Kept **native fixed-workspace one-shot APIs** from phase 64.
- Added example and test coverage for wrapper workspace use on raw/zlib/gzip wrappers.

## Capability matrix (honest snapshot)

| Capability | Status |
|---|---|
| raw deflate wrapper | yes |
| zlib wrapper | yes |
| gzip wrapper | yes |
| preset dictionary | yes |
| dynamic huffman encode | yes |
| fixed huffman encode | yes |
| streaming encode | yes |
| streaming decode | yes |
| native one-shot fixed-workspace | yes |
| wrapper-stream fixed-workspace helper | yes (caller-provided workspace) |
| protocol89 no-runtime-heap in ZRAGF-owned C paths | done (static arena + caller workspaces) |
| allocator configurable globally | yes |
| allocator configurable per stream | yes |
| C89 clean build | not yet |
| public reproducible benchmark suite | partial |

## Recommended next work

1. Keep byte-exact golden-master coverage growing as new codec paths are added.
2. Start a real `-std=c89 -pedantic` cleanup pass and capture failures in CI.
3. Add more external corpus/interop vectors for PNG/TIFF/image-centric data.
4. Keep tuning ratio/speed against zlib/libdeflate/miniz on reproducible corpora.


## Phase 66
- Added an opt-in `ZRAGF_ENABLE_C89=ON` build that sets the library target to C90.
- Removed the remaining C99-only `inline`/`stdint.h` usage from library sources.
- Added a C89 smoke example and a dedicated smoke test target.
- Current status: library + examples compile in opt-in C89 mode; most tests/benches/fuzzers still assume newer C for convenience.


## Phase 67
- Added a public benchmark suite executable: `zragf_bench_phase67_public`.
- Added deterministic built-in corpora for text, JSON, logs, PNG-like scanlines, photo-like buffers, normal-map-like buffers and binary data.
- Added CSV and Markdown summary output for reproducible result capture.
- Wired optional libdeflate comparison in the benchmark suite when the library is available at build time.
- Added a helper runner script `tools/run_phase67_public.py` and benchmark docs in `docs/BENCHMARKS.md`.
- Current status: public benchmark infra is now real; miniz comparison remains optional/follow-up depending on how miniz is supplied to the build.


## Phase 68

- benchmark suite now has versioned external corpus manifests for PNG/TIFF/text/binary sources
- optional miniz comparator is wired into the public benchmark build when miniz is available
- local smoke manifest added for offline CTest coverage


## Phase 69

- Added a locked-corpora flow on top of the public benchmark suite.
- `tools/fetch_phase69_corpora.py` now writes a content-hash lockfile and can freeze a manifest with concrete pins.
- Added local PNG/TIFF differential validation through `tools/validate_phase69_png_tiff.py`.
- Added an offline locked smoke corpus and an end-to-end CTest path for the phase 69 runner.
- Current status: reproducible corpus locking and local image validation are now wired; populating the external lockfile still depends on running the fetcher on a networked machine.


## Phase 70
- Added cache-first corpus materialization for externally downloaded corpora via `tools/fetch_phase70_corpora.py`.
- Added stronger local PNG/TIFF structural validation in `tools/validate_phase70_png_tiff.py`.
- Added a local locked phase 70 corpus and cache-materialization smoke tests.
