# External and locked corpora for phases 68–69

Phase 68 added versioned corpus manifests for the public comparative benchmark.
Phase 69 adds a **content-hash lockfile** flow and a small offline locked corpus
for smoke testing.

## Included manifests

- `phase68_public_manifest.json` pins three real-world external sources:
  - the Canterbury Corpus (`cantrbry.zip`) for text/binary/mixed files
  - the LibTIFF test image archive (`pics-3.8.0.tar.gz`) for TIFF cases
  - the official PngSuite site, with a GitHub mirror fallback, for PNG cases
- `phase68_local_smoke_manifest.json` is the original tiny local fixture used by CTest.
- `phase69_public_manifest.json` is the **phase 69** source manifest prepared for
  lockfile generation and frozen-manifest export.
- `phase69_local_locked_manifest.json` is an offline locked smoke corpus with
  expected SHA-256 values for each file.

## Fetching and locking (phase 69)

Use the helper script:

```sh
python3 tools/fetch_phase69_corpora.py \
  --manifest bench/corpora/phase69_public_manifest.json \
  --root bench/corpora/downloaded
```

That emits `phase69_corpus_lock.json` with concrete sizes and SHA-256 hashes for
whatever was actually materialized. It can also freeze a manifest with the
observed hashes:

```sh
python3 tools/fetch_phase69_corpora.py \
  --manifest bench/corpora/phase69_public_manifest.json \
  --root bench/corpora/downloaded \
  --freeze-manifest-out bench/corpora/phase69_public_frozen.json
```

## Offline smoke path

To exercise the locked flow without network access:

```sh
python3 tools/run_phase69_public.py \
  --exe build/zragf_bench_phase69_public \
  --manifest bench/corpora/phase69_local_locked_manifest.json \
  --corpus-root bench/corpora \
  --no-builtins --quick
```


## Phase 70 cache-first workflow

`phase70_public_manifest.json` keeps official URLs plus `cache_names` so the fetcher can materialize corpora from a local cache directory when the benchmark environment has no network access.
