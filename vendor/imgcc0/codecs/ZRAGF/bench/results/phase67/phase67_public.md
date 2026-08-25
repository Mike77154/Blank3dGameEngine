# ZRAGFLIB public benchmark suite (phase 67)

- zragf_version: `zragf/compat-0.67.0`
- zragf_build_config: `zragf/compat-0.67.0;c_standard=c89-opt-in+c99-default;alloc=global+stream;workspace=native-oneshot+wrapper-stream;wrappers=raw,zlib,gzip;preset_dict=yes;streaming=yes;c89=library+examples;bench=public-suite`
- zlib_runtime: `1.3.1`
- libdeflate: available
- miniz: not wired in this build (hook pending external source)
- compiler: `14.2.0`
- cpu: `unknown`
- repeats: `2`
- warmups: `1`
- builtins_used: `yes`
- external_files: `0`

## Aggregate results

| codec | level | total input | total output | weighted ratio | compress MiB/s | decompress MiB/s | rows |
|---|---:|---:|---:|---:|---:|---:|---:|
| zragf | 1 | 1409536 | 676029 | 0.479611 | 7.681 | 0.827 | 7 |
| zlib | 1 | 1409536 | 689057 | 0.488854 | 67.212 | 0.000 | 7 |
| libdeflate | 1 | 1409536 | 692542 | 0.491326 | 89.616 | 0.000 | 7 |
| zragf | 6 | 1409536 | 675853 | 0.479486 | 2.536 | 0.835 | 7 |
| zlib | 6 | 1409536 | 680777 | 0.482980 | 67.212 | 134.424 | 7 |
| libdeflate | 6 | 1409536 | 666457 | 0.472820 | 134.424 | 0.000 | 7 |
| zragf | 9 | 1409536 | 675848 | 0.479483 | 2.444 | 0.827 | 7 |
| zlib | 9 | 1409536 | 675644 | 0.479338 | 53.770 | 268.848 | 7 |
| libdeflate | 9 | 1409536 | 663884 | 0.470995 | 44.808 | 0.000 | 7 |

## Notes

- This suite compares one-shot **zlib-wrapper** compression because it is the cleanest common surface across zragflib, zlib and libdeflate.
- zragflib reports wrapper workspace bounds; external libraries are left as `-1` when not known by a stable public formula in this suite.
- Synthetic builtins are deterministic. You can also pass external files on the command line for real corpora.
