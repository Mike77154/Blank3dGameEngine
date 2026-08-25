# psd89 C89 fixed-point sanitation verification

This directory records the differential checks used for the sanitation pass.

## Input infrastructure preserved

The supplied archive contained 85 files. The sanitation pass retained every supplied path, including:

- 11 library source files and the public/internal headers
- 12 C regression/corpus-runner sources
- 14 checked-in PSD corpus fixtures
- 6 C examples and their supplied fixtures/binaries
- 9 top-level documentation/log/status files
- 1 patch file
- the Makefile and all supplied test/example executables

The supplied archive did **not** contain a `fuzz/`, CI, Conan, or vcpkg tree, so none of those were removed.

## Protocol changes

- Native `long long`, floating-point scalar types, and floating-point arithmetic were removed.
- Q16.16/Q8.24 operations use 32-bit limbs/pairs only.
- PSD 8-byte IEEE scalar fields are decoded/encoded with two 32-bit halves; no native floating scalar is required.
- PSD descriptor 8-byte integer payloads remain represented as `comp_hi` + `comp_lo` 32-bit halves.
- Public/internal offsets and byte counts now use `psd89_u32`; the library requires a 32-bit `unsigned int` instead of relying on ABI-dependent `unsigned long` width.
- ZIP/ZIP+prediction keeps zlib for byte-for-byte compatibility, with its allocator redirected to the existing fixed arena and a no-op release callback; the psd89 code does not request heap allocation.
- `make clean` was corrected so it no longer deletes checked-in PSD corpus/fixture files.

## Verification performed

1. Baseline built with `-std=c89 -Wall -Wextra -pedantic` and all supplied tests passed.
2. Sanitized tree built with the same flags and all supplied tests passed.
3. All six supplied examples were run in both trees.
4. SHA-256 was computed for 28 resulting PSD files in each tree; `baseline_outputs.sha256` and `sanitized_outputs.sha256` are identical.
5. A temporary differential harness compared Q16 multiply/divide and Q24 multiply for 200,000 deterministic full-range input pairs. Output was byte-identical.
6. A second temporary differential harness compared PSD IEEE-8-byte <-> Q16 conversion for 200,000 deterministic finite patterns/values. Output was byte-identical.

The temporary differential harness binaries/data were verification-only and are not part of the library source tree.
