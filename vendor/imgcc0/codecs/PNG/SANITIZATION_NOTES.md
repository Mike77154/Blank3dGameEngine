# Sanitation notes

## Memory conversion

Every runtime dynamic allocation site was redirected to `png_mem89`, a fixed static arena with block reuse and coalescing. Resize operations first attempt in-place growth; otherwise they allocate another arena block, copy bytes, and release the prior block back into the same arena.

The recheck found and fixed an alignment defect in the first sanitation pass: 4-byte block alignment was insufficient on LP64 validation hosts where codec structs containing pointers require 8-byte alignment. The arena now forces base alignment through a C89 union and rounds every block/header boundary to the maximum target pointer/integer alignment. The corrected build passes UBSan/ASan on the migrated original test suite.

The zlib bridge no longer relies on zlib's default allocator. `png_zlib.c` supplies arena-backed `zalloc` and `zfree` callbacks.

## Numeric conversion

The codec-owned 32-bit integer aliases are explicitly `unsigned int` / `signed int`, avoiding LP64's 64-bit `long`.

Real-number metadata and transforms use `png_fixed89` (signed Q16.16). Gamma application uses an integer log2/exp2 approximation implemented with 32-bit arithmetic only. Regression inputs containing gAMA, including 16-bit Adam7 images, decoded byte-identically to the original implementation in the supplied test matrix.

## File loading

File loading no longer stores `ftell()`/`size_t` results. Files are read incrementally into the static arena using 32-bit codec counters.

## API compatibility

Release functions keep historical names such as `png_free_image`, `png_free_apng`, and `png_decoder_free`. They are compatibility names only; implementation releases static-arena blocks and never invokes libc heap deallocation.


## Equivalence caveat

Q16.16 cannot represent every five-decimal PNG metadata value exactly. Therefore encoding `gAMA`, `cHRM`, or similar real-valued metadata may produce a nearby scaled chunk integer rather than a bit-for-bit identical chunk to the former floating-point encoder. This is a numeric-representation change, not a decoder pixel regression. The included before/after decoder corpus remains byte-identical at the output-buffer level.


## Development infrastructure restoration

The first compact sanitation archive omitted non-runtime material. The full package restores
all files from the supplied source archive unless they are superseded by a sanitized equivalent.
Binary fuzz corpus seeds and PNG fixtures are preserved byte-for-byte. C fuzz/test/differential
harnesses were migrated to `png_mem89`, `png_u32`, and `png_fixed89` rather than being dropped.
CI/build scripts now force C89 on project C sources.

The upstream archive contained no Conan recipe and no vcpkg manifest/port metadata; therefore
none could be restored. This is recorded explicitly rather than inventing packaging files that
were not present in the supplied source.
