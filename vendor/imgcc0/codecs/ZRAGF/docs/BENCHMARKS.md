# Public benchmark suite

Phase 68 extends the reproducible comparative benchmark executable for the one-shot
zlib-wrapper surface with **versioned external corpus manifests** and **optional miniz integration**.

## What it measures

For each dataset and compression level, the suite records:

- input size
- compressed size
- compression ratio
- compression time
- decompression time
- compression MiB/s
- decompression MiB/s
- zragflib wrapper workspace bounds (when available)

## Why the default benchmark uses the zlib wrapper

This is the cleanest common surface already wired in this tree for:

- zragflib
- zlib
- libdeflate (whole-buffer only)

Miniz is now supported as an **optional comparator** when `miniz` is available
through `ZRAGF_MINIZ_ROOT` or a vendored `third_party/miniz` drop.

## Built-in synthetic datasets

The executable ships with deterministic synthetic corpora that approximate:

- text
- JSON-like text
- log-like text
- PNG-like scanlines
- photo-like buffers
- normal-map-like buffers
- binary/random buffers

You can also pass external files on the command line.

## Build

```sh
cmake -S . -B build -DZLIB_USE_STATIC_LIBS=OFF
cmake --build build --target zragf_bench_phase67_public
```

## Run

```sh
./build/zragf_bench_phase67_public \
  --csv bench/results/phase67_public.csv \
  --summary bench/results/phase67_public.md \
  --repeat 5 --warmup 1
```

Quick smoke run:

```sh
./build/zragf_bench_phase67_public --quick
```

Using external corpus files:

```sh
./build/zragf_bench_phase67_public \
  --csv bench/results/files.csv \
  --summary bench/results/files.md \
  samples/a.png samples/b.json samples/c.tiff
```

## CSV schema

```text
codec,wrapper,level,dataset,source_kind,family,size_class,input_bytes,compressed_bytes,ratio,compress_ms,decompress_ms,compress_mib_s,decompress_mib_s,comp_mem_bytes,decomp_mem_bytes,ok,note
```

## Helper script

```sh
python3 tools/run_phase67_public.py --exe build/zragf_bench_phase67_public
```

## Versioned external corpora (phase 68)

Phase 68 adds manifest-driven external corpora under `bench/corpora/`:

- `phase68_public_manifest.json` for real PNG/TIFF/text/binary sources
- `phase68_local_smoke_manifest.json` for offline CTest coverage

Fetch the real corpora with:

```sh
python3 tools/fetch_phase68_corpora.py \
  --manifest bench/corpora/phase68_public_manifest.json \
  --root bench/corpora/downloaded
```

Run the phase 68 benchmark using that manifest:

```sh
python3 tools/run_phase68_public.py \
  --exe build/zragf_bench_phase68_public \
  --manifest bench/corpora/phase68_public_manifest.json \
  --corpus-root bench/corpora/downloaded \
  --fetch
```

## Optional miniz integration (phase 68)

If `miniz` is vendored or discoverable, the public benchmark adds a `miniz` codec
column automatically. The intended pinned reference for this phase is miniz release
`3.1.0`.

```sh
cmake -S . -B build -DZRAGF_BUILD_PUBLIC_BENCH=ON -DZRAGF_MINIZ_ROOT=/path/to/miniz
cmake --build build --target zragf_bench_phase68_public
```


## Locked corpora + PNG/TIFF differential validation (phase 69)

Phase 69 adds a second layer on top of the public suite:

- a **content-hash lockfile** emitted for every benchmark run
- an optional **frozen manifest** with concrete `sha256`/size pins
- a local **PNG/TIFF differential validation** step for the files present in the manifest

New manifests and tools:

- `bench/corpora/phase69_public_manifest.json` — external source manifest prepared for locking
- `bench/corpora/phase69_local_locked_manifest.json` — offline locked smoke corpus
- `tools/fetch_phase69_corpora.py` — verifies/fetches data and writes `phase69_corpus_lock.json`
- `tools/validate_phase69_png_tiff.py` — checks PNG/TIFF files with local readers
- `tools/run_phase69_public.py` — runs fetch/lock + bench + validation and writes run metadata

Offline smoke run:

```sh
python3 tools/run_phase69_public.py \
  --exe build/zragf_bench_phase69_public \
  --manifest bench/corpora/phase69_local_locked_manifest.json \
  --corpus-root bench/corpora \
  --no-builtins --quick
```

When external corpora are available, the fetcher can also emit a frozen manifest:

```sh
python3 tools/fetch_phase69_corpora.py \
  --manifest bench/corpora/phase69_public_manifest.json \
  --root bench/corpora/downloaded \
  --freeze-manifest-out bench/corpora/phase69_public_frozen.json
```


## Phase 70 real-corpus materialization

Phase 70 adds `tools/fetch_phase70_corpora.py` and `tools/run_phase70_public.py` for cache-first materialization of externally downloaded corpora. In restricted environments, put official downloads into `bench/corpora/cache/` and run with `--cache-only`.
