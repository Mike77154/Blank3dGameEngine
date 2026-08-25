# WOFF2 decoder support limits

This package is a fixed-buffer WOFF2 decoder core for single SFNT fonts.

## Supported now

- C89 style source.
- No heap calls inside the core library.
- No floating point source tokens in code.
- Big-endian WOFF2 header parsing.
- UIntBase128 parser with leading-zero and overflow rejection.
- 255UInt16 parser.
- Known tag table, including arbitrary 4-byte custom tags.
- Single SFNT fonts (`.ttf` / `.otf`), not TTC collections.
- Null-transformed tables.
- Real transformed `glyf` version 0 reverse transform.
- Real transformed `loca` version 0 reconstruction.
- Output SFNT table directory generation.
- Table sorting by tag.
- OpenType checksum generation and `head.checkSumAdjustment` patching.
- Brotli decompression via caller-supplied callback.
- Optional Google Brotli adapter using a fixed static arena when `W2F_USE_GOOGLE_BROTLI` is defined.

## Rejected cleanly

- WOFF2 collections (`flavor == 'ttcf'`).
- Transformed `hmtx` version 1.
- Unknown transforms.
- Bad `UIntBase128` encodings.
- Malformed transformed `glyf` streams.
- Malformed transformed `loca` placeholders.
- Mismatched Brotli output lengths and truncated files.

## glyf/loca transform notes

The decoder rebuilds `glyf` from the WOFF2 substreams and generates `loca` offsets during glyph emission. It does not attempt to make the output byte-for-byte identical to the original input font; the WOFF2 spec explicitly permits equivalent reconstructed glyph encodings. The output uses straightforward TrueType coordinate encoding without repeat-flag optimization.

For short `loca` format, the decoder pads odd glyph offsets by one zero byte so `offset / 2` remains valid.

## Next bite

The remaining major optional transform is `hmtx` version 1, which reconstructs missing side-bearing arrays from `glyf` xMin values and requires reading `hhea.numberOfHMetrics` plus `maxp.numGlyphs`.
