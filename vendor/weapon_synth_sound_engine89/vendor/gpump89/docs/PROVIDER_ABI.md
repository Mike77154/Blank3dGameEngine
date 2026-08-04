# Provider ABI

```c
typedef int (*gpump89_grain_provider)(
    void *user,
    int stage,
    gpump89_grain *out_grain);
```

Return non-zero and fill:

- `samples`: persistent signed 8-bit mono PCM
- `length`: number of samples
- `sample_rate`: source rate

Return zero for built-in fallback. No allocation or ownership transfer occurs.

The core applies event timing, intensity, deterministic pitch variation and mixing.
For friction stages, automatic-cycle mode stretches the supplied grain to the
scheduled motion duration.
