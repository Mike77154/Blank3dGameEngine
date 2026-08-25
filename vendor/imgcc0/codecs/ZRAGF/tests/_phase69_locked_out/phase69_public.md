# ZRAGFLIB public benchmark suite (phase 69)

- zragf_version: `zragf/compat-0.70.0`
- zragf_build_config: `zragf/compat-0.70.0;c_standard=c89;memory=static-arena+workspace;workspace=native-oneshot+wrapper-stream;wrappers=raw,zlib,gzip;preset_dict=yes;streaming=yes;c89=library+examples;bench=public-suite+locked-corpora+png-tiff-validation+real-corpus-materialization+optional-miniz`
- zlib_runtime: `1.3.1`
- libdeflate: available
- miniz: unavailable (set ZRAGF_MINIZ_ROOT or vendor third_party/miniz)
- compiler: `14.2.0`
- cpu: `AMD EPYC 9V74 80-Core Processor`
- repeats: `1`
- warmups: `0`
- builtins_used: `no`
- external_files: `4`

## Aggregate results

| codec | level | total input | total output | weighted ratio | compress MiB/s | decompress MiB/s | rows |
|---|---:|---:|---:|---:|---:|---:|---:|
| zragf | 1 | 932 | 873 | 936 | 0 | 0 | 4 |
| zlib | 1 | 932 | 849 | 910 | 0 | 0 | 4 |
| libdeflate | 1 | 932 | 877 | 940 | 0 | 0 | 4 |
| zragf | 6 | 932 | 873 | 936 | 0 | 0 | 4 |
| zlib | 6 | 932 | 843 | 904 | 0 | 0 | 4 |
| libdeflate | 6 | 932 | 842 | 903 | 0 | 0 | 4 |
| zragf | 9 | 932 | 873 | 936 | 0 | 0 | 4 |
| zlib | 9 | 932 | 843 | 904 | 0 | 0 | 4 |
| libdeflate | 9 | 932 | 842 | 903 | 0 | 0 | 4 |

## Notes

- This suite compares one-shot **zlib-wrapper** compression because it is the cleanest common surface across zragflib, zlib, libdeflate, and optional miniz.
- zragflib reports wrapper workspace bounds; external libraries are left as `-1` when not known by a stable public formula in this suite.
- Synthetic builtins are deterministic. You can also pass external files on the command line for real corpora.
