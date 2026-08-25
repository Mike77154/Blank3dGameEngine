# ZRAGFLIB public benchmark suite (phase 68)

- zragf_version: `zragf/compat-0.68.0`
- zragf_build_config: `zragf/compat-0.68.0;c_standard=c89-opt-in+c99-default;alloc=global+stream;workspace=native-oneshot+wrapper-stream;wrappers=raw,zlib,gzip;preset_dict=yes;streaming=yes;c89=library+examples;bench=public-suite+external-corpora+optional-miniz`
- zlib_runtime: `1.3.1`
- libdeflate: available
- miniz: unavailable (set ZRAGF_MINIZ_ROOT or vendor third_party/miniz)
- compiler: `14.2.0`
- cpu: `unknown`
- repeats: `1`
- warmups: `0`
- builtins_used: `no`
- external_files: `4`

## Aggregate results

| codec | level | total input | total output | weighted ratio | compress MiB/s | decompress MiB/s | rows |
|---|---:|---:|---:|---:|---:|---:|---:|
| zragf | 1 | 386 | 404 | 1.046632 | 0.037 | 0.037 | 4 |
| zlib | 1 | 386 | 400 | 1.036269 | 0.000 | 0.000 | 4 |
| libdeflate | 1 | 386 | 425 | 1.101036 | 0.000 | 0.000 | 4 |
| zragf | 6 | 386 | 404 | 1.046632 | 0.000 | 0.000 | 4 |
| zlib | 6 | 386 | 400 | 1.036269 | 0.000 | 0.000 | 4 |
| libdeflate | 6 | 386 | 408 | 1.056995 | 0.000 | 0.000 | 4 |
| zragf | 9 | 386 | 404 | 1.046632 | 0.000 | 0.000 | 4 |
| zlib | 9 | 386 | 400 | 1.036269 | 0.000 | 0.000 | 4 |
| libdeflate | 9 | 386 | 400 | 1.036269 | 0.000 | 0.000 | 4 |

## Notes

- This suite compares one-shot **zlib-wrapper** compression because it is the cleanest common surface across zragflib, zlib, libdeflate, and optional miniz.
- zragflib reports wrapper workspace bounds; external libraries are left as `-1` when not known by a stable public formula in this suite.
- Synthetic builtins are deterministic. You can also pass external files on the command line for real corpora.
