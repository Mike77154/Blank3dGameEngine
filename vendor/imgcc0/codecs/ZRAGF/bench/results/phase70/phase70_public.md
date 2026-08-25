# ZRAGFLIB public benchmark suite (phase 70)

- zragf_version: `zragf/compat-0.70.0`
- zragf_build_config: `zragf/compat-0.70.0;c_standard=c89-opt-in+c99-default;alloc=global+stream;workspace=native-oneshot+wrapper-stream;wrappers=raw,zlib,gzip;preset_dict=yes;streaming=yes;c89=library+examples;bench=public-suite+locked-corpora+png-tiff-validation+real-corpus-materialization+optional-miniz`
- zlib_runtime: `1.3.1`
- libdeflate: available
- miniz: unavailable (set ZRAGF_MINIZ_ROOT or vendor third_party/miniz)
- compiler: `14.2.0`
- cpu: `unknown`
- repeats: `1`
- warmups: `0`
- builtins_used: `no`
- external_files: `6`

## Aggregate results

| codec | level | total input | total output | weighted ratio | compress MiB/s | decompress MiB/s | rows |
|---|---:|---:|---:|---:|---:|---:|---:|
| zragf | 1 | 2700 | 1566 | 0.580000 | 0.000 | 0.000 | 6 |
| zlib | 1 | 2700 | 1540 | 0.570370 | 0.000 | 0.000 | 6 |
| libdeflate | 1 | 2700 | 1570 | 0.581481 | 0.000 | 0.000 | 6 |
| zragf | 6 | 2700 | 1565 | 0.579630 | 0.000 | 0.000 | 6 |
| zlib | 6 | 2700 | 1528 | 0.565926 | 0.000 | 0.000 | 6 |
| libdeflate | 6 | 2700 | 1535 | 0.568519 | 0.000 | 0.000 | 6 |
| zragf | 9 | 2700 | 1565 | 0.579630 | 0.000 | 0.257 | 6 |
| zlib | 9 | 2700 | 1528 | 0.565926 | 0.000 | 0.000 | 6 |
| libdeflate | 9 | 2700 | 1535 | 0.568519 | 0.000 | 0.000 | 6 |

## Notes

- This suite compares one-shot **zlib-wrapper** compression because it is the cleanest common surface across zragflib, zlib, libdeflate, and optional miniz.
- zragflib reports wrapper workspace bounds; external libraries are left as `-1` when not known by a stable public formula in this suite.
- Synthetic builtins are deterministic. You can also pass external files on the command line for real corpora.
