# rawmix phase 3 notes

This phase adds three things without changing the core rules:

1. an **optional backend adapter** layer
2. **bus FX inserts**
3. **quality-selectable resampling**

## 1) Optional backend adapter

The mixer core remains host-driven and device-agnostic.

To keep that clean boundary, the native backend integration lives in:

- `extras/rawmix_miniaudio_adapter.h`
- `extras/rawmix_miniaudio_adapter.c`

That bridge maps miniaudio's playback/duplex callback into the existing:

- `rm_engine_render_s16()` / output path semantics
- `rm_engine_process_duplex_s16()` / capture+playback semantics

The core still does not own devices or threads.

## 2) Bus FX inserts

Each bus now has a fixed insert chain:

- compile-time slot count: `RAWMIX_MAX_BUS_FX`
- no dynamic allocation
- effect state stored inside `rm_engine`

Built-in effects in this revision:

- low-pass
- drive / saturating shaper

This is deliberately small but structural: the important step was getting a stable insert-chain model into the bus path.

## 3) Resampler selection

Voices can now request a resampler mode:

- nearest
- linear
- cubic / HQ

The engine also stores a default resampler.

### Current behavior

- **buffer voices**: nearest, linear, cubic are implemented
- **stream voices**: nearest and linear are implemented; cubic currently falls back to the linear path

That tradeoff keeps the stream state compact while still giving buffer playback a higher-quality mode for offline assets and mismatched sample-rate material.

## Stats added

- `bus_fx_frames`
- `resampler_hq_frames`

These are simple counters meant for smoke tests and rough profiling, not deep telemetry.

## Why this shape

The point of phase 3 was not to turn rawmix into a giant engine overnight.
The point was to add the **structural seams** you want before the codebase gets crowded:

- devices outside the core
- processing at the bus level
- pluggable resampling quality

That makes the next steps easier:

- more filters
- better HQ resamplers
- alternate backend bridges
- optional decoder/resource layers
