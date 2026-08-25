Phase bundle status
==================

Included up to: phase15

Highlights of this bundle:
- modular RIFF/WebP parser + demux/mux
- VP8L decode
- VP8 still decode pipeline + ALPH support
- conformance harness for decode
- animated WebP decode/composition
- animated WebP mux / assembler
- raw-frame encode (lossless-first)
- phase11 encoder work:
  - VP8L compression upgrades: near-lossless, subtract-green, color-cache, limited backrefs
  - optional cwebp-backed lossy still encode bridge
  - optional mixed lossy/lossless frame selection in anim path
  - encode-oracle scripts for cwebp / img2webp / gif2webp
- phase12 groundwork:
  - native VP8 lossy planning API with per-macroblock stats and suggestions
  - better CLI/tool parity for preset/filter/alpha-q/sharp-yuv/kmin/kmax/loop/min-size
  - animation keyframe scheduler corrected so non-min-size mode is no longer all-keyframes

Not fully closed yet:
- self-hosted VP8 lossy bitstream emitter in pure C89
- advanced mixed heuristics comparable to full official anim encoder
- richer VP8L transforms beyond the current compression upgrades

- phase13 groundwork turned into a native intra-only emitter foundation
  - per-cell intra IR (`16x16` / `8x8` / `4x4`) with heuristic qindex
  - still lossy path works without cwebp via native proxy output
  - animated lossy path works without external tools via the same proxy route

Still not fully closed:
- standards-complete `VP8 ` bitstream emission in pure C89
- bool encoder / token writer / full intra-mode bitstream serialization


## Phase 14
Native VP8 bool/tree/token writer landed for opaque lossy still paths, with a narrow intra-only subset, plus encode-conformance scaffolding against official tools.


## Phase 15
Native VP8 intra-only writer now adds multi-token-partition output, segmentation, coeff-skip, heuristic coeff-probability updates, richer 4x4 B_PRED search and UV mode search, plus a legacy-vs-phase15-vs-cwebp conformance harness.


## Phase 16
- native writer now adds a real intra16/Y2 path for smooth macroblocks
- sparse AC residual coding landed for native luma/chroma block decisions
- phase16 regression coverage checks intra16 activation, Y2 signalling and lossy round-trip validity
