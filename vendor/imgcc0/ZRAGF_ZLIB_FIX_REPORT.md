# ZRAGF zlib/DEFLATE compatibility repair

## Scope

This revision repairs the vendored Protocol89 ZRAGF implementation so the imgcc0 image-decoder build can route PNG/APNG, TIFF and PSD DEFLATE/zlib decoding through ZRAGF without the system zlib library.

The supplied ZRAGF tree is preserved except for one intentional source patch: `codecs/ZRAGF/zragf_stream.c`. `vendor_manifests/ZRAGF.sha256` remains the manifest of the supplied package; `vendor_manifests/ZRAGF_PROTOCOL89_FIXED.sha256` records the repaired tree. `make vendor-verify` requires the upstream-to-repaired manifest delta to be exactly that one file.

## Root cause 1: exact-fill output before EOB

The inflater previously returned NEED_OUTPUT before decoding the next literal/length symbol whenever `avail_out == 0`. That is incorrect when the caller buffer has been filled exactly by the last output byte: the next symbol can be end-of-block (EOB), which produces no output. The decoder must still be able to consume EOB, parse a following final empty block, and validate the zlib trailer/checksum.

The output-space guard now occurs only after a decoded symbol is known to require output. EOB can advance the state machine with zero output space.

Permanent regression: `tests/test_zragf_zlib_compat_img.c` contains a valid zlib stream consisting of a non-final stored block carrying four bytes followed by an empty final stored block. The destination buffer is exactly four bytes. The test requires `ZRAGF_STREAM_END`, zero remaining output capacity, and exact bytes 01 02 03 04.

## Root cause 2: zlib level 0 semantics

ZRAGF accepted `level=0` but did not force stored DEFLATE blocks. The TIFF regression constructs a level-0 stream and depends on zlib-compatible stored-block behavior. The deflate stream path now passes `force_stored` whenever `st->level == 0` for normal data blocks and flush/finish data blocks.

Permanent regression: the same test compresses bytes 01 02 03 04 with zlib wrapper, level 0, and requires the exact short zlib/stored stream bytes.

## imgcc0 cutover

`compat/zlib.h` maps the zlib-shaped calls used by TIFF and PSD to ZRAGF. The top-level build has no `-lz` dependency. PNG/APNG continue to use the existing explicit imgcc0-to-ZRAGF bridge.

Decode regression status:

- strict protocol audit: PASS;
- ZRAGF Protocol89 audit: PASS;
- dedicated ZRAGF/zlib image compatibility tests: PASS;
- complete TIFF vendor test suite through ZRAGF: PASS, including the empty-final-block case;
- system-zlib-generated empty-final TIFF decoded through old zlib and repaired ZRAGF: identical PAM bytes;
- PSD direct-vendor vs imgcc0/ZRAGF facade: 24/24 canonical RGBA outputs byte-identical;
- PNG/APNG/DDS/BMP/PCX adapter regression: 9/9 byte-identical;
- legacy JPEG/TGA/QOI/TIFF/GIF/WebP regression: 13/13 frames byte-identical.

## Encoder note

The repair makes level 0 stored behavior zlib-compatible for the covered regression. Higher compression levels are not claimed to reproduce zlib's compressed byte stream. DEFLATE permits multiple valid block choices, and zlib's higher-level output depends on its own match-finding, block selection and strategy heuristics. Decoder output parity is therefore the production gate for imgcc0; encoder bitstream identity remains a separate compatibility project.
