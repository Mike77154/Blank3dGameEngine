# tiny_png — C89 / fixed89 sanitation fork

This fork takes the supplied PNG/APNG codec and applies a strict static-memory C89 profile.

## Hard constraints implemented

- C89 (`-std=c89 -pedantic`)
- no libc dynamic allocation calls
- no dynamic heap
- no 64-bit integer types in codec-owned C/H sources
- no floating-point types in codec-owned C/H sources
- 32-bit `png_u32` / `png_i32` use `unsigned int` / `signed int`
- metadata that previously required real-number storage now uses signed **Q16.16** `png_fixed89`
- zlib allocation is redirected through the codec's static arena (`zalloc` / `zfree` callbacks)

Compatibility cleanup/release API names such as `png_free_image()` remain so old call sites do not have to change, but they do **not** call libc `free()`; they return blocks to the static arena.

## Static arena

`png_mem89.c` owns a fixed static arena. Default capacity:

```c
#define PNG_MEM89_ARENA_BYTES (64u * 1024u * 1024u)
```

Override it at compile time if required:

```sh
gcc -DPNG_MEM89_ARENA_BYTES='(32u*1024u*1024u)' ...
```

The allocator supports reusable blocks and in-arena resize-by-grow/copy. Arena storage and every block payload are aligned to the maximum of the target data-pointer, function-pointer, and `unsigned int` sizes using C89-compatible logic. `png_mem89_reset()` invalidates every outstanding arena pointer and must only be called when no codec object is alive.

## Fixed-point metadata

`png_fixed89` is signed Q16.16 (`65536 == 1.0`). Helpers are in `png_fixed89.h`:

```c
png_fixed89 g;
g = png_fixed89_from_ratio(45455, 100000); /* PNG gAMA 0.45455 */
```

Public field names were kept where practical, but fields that formerly represented non-integer metadata now store Q16.16.

## Build

Linux/MSYS2-like shell:

```sh
./build_c89.sh
```

Or directly:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 -DPNG_DEC_USE_ZLIB -I. \
  -c png_mem89.c png_fixed89.c png_crc.c png_filters.c png_chunks.c \
  png_parser.c png_render.c png_stubs.c png_zlib.c png_decoder.c \
  png_apng.c png_apng_progressive.c png_encoder.c
```

Link consumers with zlib (`-lz`). The codec supplies zlib with static-arena allocation callbacks.

## Before/after byte regression

The sanitation was compared against the uploaded original build by decoding both into a canonical binary dump containing dimensions/rowbytes plus the complete RGBA8 pixel buffer.

Results included in `tests/reports/`:

- 35 unique valid PNG inputs, each tested in two decode modes: **70/70 byte-identical**
- 5 unique malformed PNG inputs, each tested in two modes: **10/10 same error result**
- full APNG decode: **3/3 valid animations byte-identical frame-by-frame**
- malformed APNG: **4/4 same error result**
- coverage includes grayscale 1/2/4/8/16, GA8/16, palette 1/2/4/8, RGB8/16, RGBA8/16, Adam7, gAMA, metadata seeds, unknown chunks, and APNG

The generated corpus in `tests/corpus/png/` was produced with the **pre-sanitation/original encoder**, then decoded by both builds.

## Runtime smoke tests

`tests/roundtrip_smoke.c` checks sanitized encode/decode roundtrip with Adam7 + gAMA. `tests/repeat_smoke.c` performs repeated file decodes and verifies that arena usage returns to zero after every image release.

A recheck also migrated the original `test_upgrade.c` metadata expectations from floating point to Q16.16 without changing the tested behaviors. All **38 original functional tests** then passed against the sanitized core. The same migrated suite passes under GCC AddressSanitizer + UndefinedBehaviorSanitizer.

## Byte-equivalence scope

Decoded image/frame dumps are compared byte-for-byte and match the original in the included matrix. Encoder output without real-valued metadata is also byte-identical in spot checks (including Adam7). When real-valued chunks such as `gAMA`/`cHRM` are authored through Q16.16 fields, the encoded chunk integer can differ slightly from the old floating-point encoder because Q16.16 quantizes the requested decimal. Pixel decoding remains byte-identical, but a whole-file bit-identical metadata roundtrip is **not** claimed.

## Important limits

The default arena is global and not thread-safe. Maximum practical image/APNG complexity is bounded by the compile-time arena size; if an operation cannot fit, the codec returns its existing memory/error path rather than allocating from a heap.


## Full-package restoration

The rechecked package restores the development/support material that was accidentally
omitted from the first compact sanitation ZIP. The following are present again:

- `.github/workflows/` ClusterFuzzLite and differential CI
- `ci/run_all_local.sh`
- `differential/` scripts, corpus list, and libpng harness
- `fuzz/` targets, dictionary, corpus generator, build scripts, and the complete original seed corpus
- `test_upgrade.c` migrated to Q16.16/static-arena operation
- `test_differential_libpng.c` migrated to Q16.16/static-arena operation
- `test_adam7_iccp.png` and `test_apng_anim.png`
- the original eventloop reports
- the newer byte-regression corpus/reports under `tests/`
- protocol/build utilities under `tools/`

The supplied original archive did **not** contain Conan or vcpkg recipe files, so there
were no Conan/vcpkg files to restore. No replacement package-manager metadata was invented.

`docs/FULL_CAPABILITY_REFERENCE.md` restores the long-form upstream feature/API documentation
with executable examples adjusted for the current fixed89 interface. `docs/UPSTREAM_README.md`
is retained as a clearly marked historical archive.

For the strongest local gate, run:

```sh
./ci/run_all_local.sh
```

It performs the protocol source audit, the migrated original upgrade suite, sanitized smoke
tests, the libpng differential suite, and all restored fuzz seed corpora through standalone targets.
