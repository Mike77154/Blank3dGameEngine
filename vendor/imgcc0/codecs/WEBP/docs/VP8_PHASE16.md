# Phase 16 — intra16/Y2 + sparse AC + tighter native writer validation

This phase moves the native VP8 writer beyond the phase15 `B_PRED`-only shape.

## Landed

- native **intra16** luma mode selection (`DC/V/H/TM`) on smooth macroblocks
- native **Y2** signalling for the intra16 path
- sparse **non-DC residuals** for native luma/chroma blocks
  - current sparse set is intentionally small and static (`DC`, `1`, `4`, `5`) to keep the writer simple and deterministic in C89
- richer stats surfaced through `gwpvp8bitstream --dump-json`
  - `y_mode_mask`
  - `y2_non_zero`
- new smoke/regression coverage in `tests/test_vp8_phase16.py`

## Intent

The goal here is not full `cwebp` parity. The goal is to cross the next structural threshold:

1. stop being `B_PRED`-only
2. emit valid `Y2` on the native writer side
3. stop being purely DC-only for block residuals
4. keep the whole path static-memory / C89 / no internal malloc

## Current shape

- still key-frame only
- still intra only
- still heuristic RD, not a full encoder search
- Y2 is currently conservative
- AC search is sparse and greedy, not exhaustive

## Why this phase helps

For flat or near-flat macroblocks, intra16+Y2 reduces mode overhead and exposes a more realistic VP8 still path.
For more textured blocks, sparse AC gives the native writer a better residual vocabulary than phase15's DC-centric shape.

## Still pending after phase16

- broader AC search / better transform-domain fitting
- stronger Y2 modelling than the current conservative path
- better RD tuning against official `cwebp`
- more exact probability tuning and partition strategy under wider corpora
