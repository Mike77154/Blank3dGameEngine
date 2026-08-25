# rawmix phase 4 notes

Phase 4 focuses on three things:

1. **real biquad inserts per bus**
2. **soft master limiting**
3. **HQ cubic resampling for stream voices**

## 1) Biquad bus insert

New API:

```c
rm_result rm_engine_bus_fx_set_biquad(rm_engine *engine,
                                      rm_u16 bus_id,
                                      rm_u16 slot,
                                      const rm_biquad_desc *desc);
```

`rm_biquad_desc` expects normalized coefficients where `a0 == 1`, stored in signed **Q14**:

```c
typedef struct rm_biquad_desc_s {
    rm_s16 b0_q14;
    rm_s16 b1_q14;
    rm_s16 b2_q14;
    rm_s16 a1_q14;
    rm_s16 a2_q14;
    rm_s16 wet_q15;
    rm_s16 output_gain_q15;
} rm_biquad_desc;
```

Processing model:

- transposed direct-form II
- separate state per channel
- dry/wet blend in Q15
- post-FX output trim in Q15

### Why raw coefficients?

The core keeps its original constraints:

- **no heap**
- **no float/double**
- **C89 only**

Because of that, coefficient design is pushed to tooling. The repo now includes:

```text
tools/rbj_biquad_coeffs.py
```

That helper computes standard cookbook coefficients and prints rawmix-ready Q14 numbers.

Example:

```sh
python3 tools/rbj_biquad_coeffs.py lowpass 48000 2000 --q 0.7071
```

## 2) Soft master limiter

New API:

```c
rm_result rm_engine_set_limiter(rm_engine *engine,
                                rm_s16 threshold_q15,
                                rm_u16 attack_frames,
                                rm_u16 release_frames,
                                rm_s16 output_gain_q15);
rm_result rm_engine_clear_limiter(rm_engine *engine);
```

Behavior:

- limiter sits **after bus summing, master gain and headroom**
- channel-linked peak detector
- attack/release smoothing in frame units
- final safety stage uses the existing soft-clip shape
- stats increment in `limiter_frames`

Design intent:

- good as a **ceiling / safety net**
- intentionally simple and deterministic
- not intended to replace a full mastering compressor/limiter with look-ahead

## 3) HQ stream resampling

Before phase 4:

- buffer voices supported cubic/HQ
- stream voices effectively used nearest/linear only

After phase 4:

- stream voices keep a 4-sample window:
  - previous
  - current
  - next
  - next+1
- `RM_RESAMPLER_CUBIC` now uses the same 4-point cubic interpolator for streams too
- `resampler_hq_frames` now counts HQ work for both buffer and stream voices

## Tests added

The smoke test now covers:

- raw biquad bus insert behavior
- limiter engagement + clipping avoidance
- cubic stream HQ path producing output different from linear

## Compatibility notes

- old phase 3 APIs remain available
- `rm_engine_bus_fx_set_lowpass()` and `rm_engine_bus_fx_set_drive()` still work
- the new biquad path is additive, not a breaking replacement
