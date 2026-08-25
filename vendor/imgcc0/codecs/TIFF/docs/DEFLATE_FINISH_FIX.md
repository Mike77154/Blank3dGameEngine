# Deflate finish fix

This patch hardens the no-heap zlib/Deflate stored-block decoder used by `Compression=8` and `Compression=32946`.

## Fixed edge case

Valid zlib streams may contain more than one DEFLATE block inside a single image segment. A segment can therefore end with:

- one stored block that carries data and has `BFINAL=1`, or
- a stored block with data and `BFINAL=0`, followed by an empty stored block with `BFINAL=1`, then the Adler-32 trailer.

Previously, `tifx_zlib_stored_decoder_finish()` expected the final block to have already been consumed during the last `read()` call. That rejected valid streams whose output finished exactly at the end of a non-final data block while a trailing empty final block still remained in the input.

## What changed

- Added a finish-path drain for trailing empty stored blocks until the real final block is consumed.
- The finish path still rejects extra non-empty stored blocks after the expected output size.
- Adler-32 validation remains mandatory.

## Regression coverage

`tests/test_tifx.c` now includes a regression test that rewrites a generated TIFF Deflate strip into:

1. a non-final stored block carrying all pixel bytes,
2. a final empty stored block,
3. the original Adler-32 trailer.

The patched decoder accepts and decodes that file successfully.
