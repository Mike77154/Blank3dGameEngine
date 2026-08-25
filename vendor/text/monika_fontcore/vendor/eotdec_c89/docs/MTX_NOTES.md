# MTX / MicroType Express notes

## Important shape

A MicroType Express stream starts with a 10-byte header:

```text
byte 0      version
bytes 1..3  copy limit / copy distance, big-endian 24-bit
bytes 4..6  offset to block 2, big-endian 24-bit
bytes 7..9  offset to block 3, big-endian 24-bit
```

Block 1 begins immediately after the header at offset 10. Blocks 1, 2, and 3 are independently LZCOMP-compressed.

The implemented stage is:

```text
MTX header + 3 LZCOMP streams -> 3 decompressed CTF blocks
```

The not-yet-implemented stage is:

```text
3 CTF blocks -> rebuilt TrueType/SFNT file
```

## LZCOMP implementation details

The decoder mirrors the W3C/Monotype reference algorithm but removes dynamic memory:

- Adaptive Huffman trees are fixed-size arrays.
- The LZ history is a static ring buffer bounded by `EOTDEC_MTX_MAX_COPY_DISTANCE`.
- Compressed input blocks are loaded into a static buffer bounded by `EOTDEC_MTX_MAX_COMPRESSED_BLOCK`.
- Optional LZCOMP run-length post-processing is decoded as a streaming state machine.
- Output is streamed directly to `FILE *` instead of accumulating in heap memory.

## CTF rebuild plan

For a complete `mtx-extract-ttf`, the next phase needs these table transforms:

1. Parse block 1 as a CTF-flavored SFNT directory.
2. Copy unchanged tables directly.
3. Rebuild `loca` from decoded glyph lengths.
4. Expand `cvt ` differential values.
5. Expand `hdmx` and `VDMX` surprise-coded values using integer prediction.
6. Decode `glyf` glyph records:
   - simple glyph contour endpoints using 255USHORT;
   - simple glyph coordinate triplets;
   - composite glyph records;
   - bbox emission;
   - instruction bytecode size restoration.
7. Interleave block 2 push data and block 3 glyph instruction data while rebuilding glyph records.
8. Recompute table checksums and `head.checkSumAdjustment`.

No floating point is required for these steps; the prediction formulas can be done with scaled integers/fixed point.
