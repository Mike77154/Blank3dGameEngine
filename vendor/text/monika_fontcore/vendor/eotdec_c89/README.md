# eotdec_c89

Embedded OpenType (EOT) decoder / extractor in strict old-school C89 style.

## Constraints

- C89 source.
- No `malloc`, `free`, `realloc`, or heap allocation.
- No `float` or `double`.
- Fixed-size static work buffers only.
- EOT wrapper is read little-endian; SFNT/OpenType payload tables are read big-endian.

## What works

### Raw / XOR EOT

```sh
make
./eottool info tests/minimal_raw_v1.eot
./eottool extract tests/minimal_raw_v1.eot out.ttf
```

`extract` writes the raw SFNT OpenType/TrueType payload for non-MTX EOT files. If `TTEMBED_XORENCRYPTDATA` is set, the payload is XOR-decoded with `0x50` while copying.

### MTX / MicroType Express EOT

MTX is a two-stage font compression path:

```text
EOT FontData
  -> MTX header
  -> 3 independent LZCOMP streams
  -> 3 CTF blocks
  -> CTF-to-TTF table reconstruction
```

This package now implements the first real MTX decompression stage: it parses the 10-byte MTX header and LZCOMP-unpacks all three compressed streams into their CTF blocks.

```sh
./eottool mtx-info font.eot
./eottool mtx-unpack font.eot decoded/font
```

That writes:

```text
decoded/font.block1_font_tables.ctf
decoded/font.block2_push_data.ctf
decoded/font.block3_glyph_insns.ctf
```

The full CTF-to-rebuilt-TTF stage is intentionally not faked yet. `extract` still returns a clean MTX message for compressed EOT, because a valid `.ttf` requires rebuilding `loca`, expanding translated `glyf`, `cvt`, `hdmx`, and `VDMX` table content, and stitching glyph push/instruction data back into TrueType glyph records.

## Static limits

Defaults are in `include/mtxdec.h`:

```c
#define EOTDEC_MTX_MAX_COMPRESSED_BLOCK (8UL * 1024UL * 1024UL)
#define EOTDEC_MTX_MAX_COPY_DISTANCE    (2UL * 1024UL * 1024UL)
#define EOTDEC_MTX_MAX_AHUFF_RANGE      512
```

Raise these at compile time if you need unusually huge MTX fonts:

```sh
make CFLAGS='-std=c89 -Wall -Wextra -pedantic -O2 -DEOTDEC_MTX_MAX_COPY_DISTANCE=4194304UL'
```

## Commands

```text
eottool info <font.eot>
eottool extract <font.eot> <out.ttf|out.otf> [--loose]
eottool mtx-info <font.eot>
eottool mtx-unpack <font.eot> <out-prefix>
```

## Files

```text
include/eotdec.h      EOT wrapper/SFNT public API
include/mtxdec.h      MTX/LZCOMP public API
src/eotdec.c          EOT parser/extractor
src/mtxdec.c          MTX header + LZCOMP no-heap decoder
tools/eottool.c       CLI
docs/SPEC_NOTES.md    EOT notes
docs/MTX_NOTES.md     MTX/LZCOMP notes and next-stage CTF plan
tests/*.eot           tiny synthetic regression samples
```

## License

CC0-style/public-domain friendly for this generated package. Check font licenses before extracting or redistributing real fonts.
