# GiffyWebP C89 - phase13 bundle

This bundle consolidates the latest implemented source tree currently available in this environment.

Included in code:
- Phase 1: modular container / VP8L foundation
- Phase 2: VP8 control headers + bool decoder base
- Phase 3: entropy/modes/token scaffolding
- Phase 4: residual / transform / reconstruction / filter kernels
- Phase 5: full-frame VP8 reconstruction pipeline
- Phase 6: precision pass + ALPH + VP8 path
- Phase 7: conformance harness + plane diff tooling
- Phase 8: animated WebP decode / compose
- Phase 9: animated WebP mux / assembly encoder
- Phase 10: raw-frame lossless-first encode
- Phase 11: VP8L compression upgrades + optional official-tool lossy bridge
- Phase 12:
  - native VP8 lossy planning API with per-macroblock stats and suggestions
  - better parity with official tool knobs (`preset`, `alpha-q`, `filter-strength`, `sharp-yuv`, `kmin`, `kmax`, `loop`, `min-size`)
  - corrected animation keyframe scheduling when `minimize_size` is disabled

Still planned after this bundle:
- raw lossy VP8 bitstream emission from pixels in pure C89
- more advanced mixed lossy/lossless animation heuristics
- richer VP8L transforms beyond the current encoder upgrades


Phase 13 note:
- the bundle now includes a native intra-only emitter foundation and a no-tool lossy proxy path for still + animation.
- the remaining step is the actual `VP8 ` bitstream writer.


## Phase 14
Native VP8 bool/tree/token writer landed for opaque lossy still paths, with a narrow intra-only subset, plus encode-conformance scaffolding against official tools.


Phase15 notes:
- the native lossy writer now has a genuine phase15 path exposed by `gwpvp8bitstream` and `gwpencode`.
- use `--legacy-native` in the examples to compare against the narrow phase14-style path.


## Phase 16
- native writer now adds a real intra16/Y2 path for smooth macroblocks
- sparse AC residual coding landed for native luma/chroma block decisions
- phase16 regression coverage checks intra16 activation, Y2 signalling and lossy round-trip validity
