# Native writer conformance

The phase15 harness compares three useful baselines when the official tools are available:
- **phase15 native writer**
- **legacy-native compatibility mode**
- **`cwebp` + `dwebp`** from the official libwebp toolchain

The harness is intentionally focused on practical questions:
- does the local writer still emit a valid `VP8 ` payload inside RIFF/WebP?
- does decode succeed locally and with official tools?
- does phase15 beat the legacy-native compatibility path on representative images?
- how far is the decoded output from the official `cwebp` reference decode?

Because the native writer is still intra-only and DC-residual-only, the harness is **not** trying to prove bit-exact equivalence with `cwebp`. It is meant to track regressions, size wins against the legacy path, and visual drift versus the official toolchain.


## Phase 16 note

The native writer now exposes `y_mode_mask` and `y2_non_zero` in the JSON probe output so conformance triage can distinguish:

- `B_PRED`-only regressions
- intra16/Y2 activation on smooth macroblocks
- sparse AC regressions vs `--dc-only` compatibility runs
