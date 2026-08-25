# Phase 14: native VP8 bool+token writer

This phase adds a native VP8 intra-only writer path for opaque images.

What landed:
- RFC-6386 style boolean encoder (`src/enc/vp8_bool_enc.*`)
- generic tree writer (`src/enc/vp8_tree_enc.*`)
- coefficient/token writer using VP8 default probabilities (`src/enc/vp8_tokens_enc.*`)
- native key-frame emitter for an intra-only subset (`src/enc/vp8_native_enc.*`)
- `gwpencode --lossy` now prefers native `VP8 ` for opaque input and falls back to the proxy/lossless bridge for transparent input
- new example: `examples/gwpvp8bitstream`

Current scope:
- key frame only
- one token partition
- segmentation disabled
- coefficient-skip disabled
- macroblock mode fixed to B_PRED with per-subblock DC predictors
- chroma mode fixed to DC
- coefficients encoded as DC-only 4x4 blocks

This is a real VP8 bitstream path, but it is intentionally narrow and compression is modest.
