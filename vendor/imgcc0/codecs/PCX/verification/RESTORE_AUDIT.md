# PCX89 full-distribution restore audit

## Restore rule
The original distribution was used as the base tree and the sanitized PCX89 codec was overlaid onto it. No original source/distribution path was deleted.

- Original uploaded distribution: 215 files, 511,309 bytes uncompressed.
- Restored PCX89 source distribution before this audit file: 233 files, 573,895 bytes uncompressed.
- Original paths missing from restored tree: 0.

## Recovered unchanged where compatible
- `.github/` CI, issue templates, rulesets and release workflows
- CMake, CMakePresets, pkg-config and CPack infrastructure
- Conan recipe and `test_package`
- vcpkg overlay port
- packaging, governance, policies, exports and release scripts
- documentation and historical release notes
- fuzz corpus, crash/fixture metadata and corpus tooling
- examples and consumer samples
- audit/release/reproducibility scripts

## Recovered and adapted to PCX89
- `Makefile`: full target surface restored; C89/static codec remains the implementation.
- `include/pcx/pcx_export.h`: DLL import/export, visibility, SAL, access, analyzer and deprecation annotations restored; dynamic-ownership annotations are inert compatibility macros.
- `exports/pcx.map` and `exports/pcx.exports.txt`: regenerated for the 105-symbol PCX89 ABI.
- `tools/pcx_diag_cli.c` and `tools/pcx_compat_cli.c`: static release/reset API.
- `tools/pcx_bench.c`: deterministic static-buffer stress benchmark; no floating-point timing or dynamic storage.
- `tests/test_smoke.c` and `tests/test_public_api.c`: static-buffer roundtrip/API tests.
- `tests/test_decoder.py`: ctypes byte-count ABI changed to 32-bit `pcx_size`; release APIs updated.
- compiler/buffer/API/portability audits: expectations updated for PCX89 static storage.
- Conan exports include installed nested headers.
- vcpkg overlay version synchronized to 1.16.0.
- fuzz harnesses added for stdin/AFL-style and libFuzzer inspect/RGB/indexed paths, using `pcx_size` and static codec output storage.

## Protocol scan
All repository `.c` / `.h` files were scanned after restoration for forbidden tokens/types used by the PCX89 protocol. Result: PASS.

Production protocol scan (`verification/protocol_scan.py`): PASS, 18 production source/header files.

## Verification executed
- strict C89 production build with warnings-as-errors: PASS
- static + shared library build: PASS
- smoke RGB/indexed roundtrip + corpus decode: PASS
- Python decoder matrix: PASS (11 generated cases plus strict/indexed/memory paths)
- CMake shared build + CTest: PASS
- CMake GCC preset: PASS
- CMake Clang preset: PASS
- pkg-config consumer: PASS
- CMake consumer: PASS
- ABI export allowlist + SONAME: PASS
- C89/C99/C11/C++11 installed-header matrix: PASS
- compiler-contract audit: PASS
- buffer-contract audit: PASS
- API surface/installed-header audit: PASS
- visibility audit: PASS
- stdin fuzz corpus seed checks for inspect/RGB/indexed: PASS
- libFuzzer build and corpus run: PASS
- Conan Python recipe syntax: PASS
- vcpkg manifest JSON parse: PASS
- GitHub Actions YAML parse: PASS

Conan and vcpkg executables were not installed in the execution environment, so package-manager end-to-end installation was not executed.

## Byte-exact decode/encode status
The six production `.c` files are SHA-256 identical to the previously byte-differential-tested PCX89 sanitized core. Therefore the existing differential record remains applicable:

- Decode comparisons: 204 / 204 PASS
- Encode comparisons: 6 / 6 PASS

See `bytecmp_results.json` and `VERIFY_SUMMARY.txt`.
