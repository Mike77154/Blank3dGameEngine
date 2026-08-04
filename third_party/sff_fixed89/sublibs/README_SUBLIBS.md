# Image/data sublibs for the SFF stack

This tree now ships three standalone C89/no-heap sublibs and wires them into `libsff.a`.

## zlib89/
- RFC 1950 zlib wrapper parsing
- RFC 1951 DEFLATE inflate
- stored / fixed Huffman / dynamic Huffman blocks
- Adler-32 verification
- no malloc, no floats, integer-only

## png89/
- PNG signature/chunk parser
- CRC32 verification
- IHDR/PLTE/tRNS/IDAT/IEND handling
- supports color types 0,2,3,4,6
- supports legal PNG bit depths 1/2/4/8/16
- defiltering for all standard filter types
- Adam7 interlace reconstruction
- indexed decode (`png89_decode_indexed8`)
- RGBA decode (`png89_decode_rgba8888`)
- optional convenience wrappers through `png89_zlib89.*`
- no malloc, no floats, integer-only

## pcx89/
- full header parser for PCX
- RLE decode
- supports common PCX variants needed in retro pipelines
- indexed 8-bit decode helper for SFF-style PCX
- RGBA8888 decode for broader use
- no malloc, no floats, integer-only

## Integration status

- `libsff.a` now embeds `pcx89`, `png89`, `png89_zlib89`, and `zlib89`
- SFF PNG decoding works without an external codec callback when a caller-supplied work buffer is provided to decode APIs
- custom PNG backends are still supported through `SffImageCodec`
