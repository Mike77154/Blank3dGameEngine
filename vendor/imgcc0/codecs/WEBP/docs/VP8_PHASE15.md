# Phase 15: native VP8 intra writer grows teeth

This phase extends the phase14 native `VP8 ` path from a very narrow proof-of-validity writer into a more capable intra-only encoder.

What landed:
- auto or forced token partition counts (`1/2/4/8`)
- usable segmentation (`1..4` segments) with per-segment quant/filter deltas
- per-macroblock `skip_coeff` signalling
- coefficient probability updates derived from a pre-pass token histogram
- real 4x4 intra mode search inside `B_PRED`
- real UV mode search (`DC/V/H/TM`)
- legacy compatibility knobs for A/B comparison from the examples
- improved native-vs-legacy-vs-`cwebp` oracle harness

Important scope note:
- still key-frame only
- still intra-only
- still no inter prediction / motion vectors
- still no Y2 / intra16 path on the native writer side
- residual writer remains DC-only per coded block

The goal of phase15 is not bit-exact parity with `cwebp`; it is to materially improve the native writer while keeping the bitstream valid and the implementation readable.
