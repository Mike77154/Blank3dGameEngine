# phase 5 notes - daw-style mixer pass

This phase pushes `rawmix` from a compact phase-4 mixer into a more serious **mix-engine** shape while preserving the hard project rules:

- C89 API
- no heap in the core
- no floating point in the core
- fixed compile-time storage

## Added in phase 5

### 1) Aux-style bus sends

`rawmix` now has a fixed send matrix:

- source bus -> destination bus
- pre-fader and post-fader modes
- no heap and no graph allocation

To keep rendering one-pass and deterministic, sends are **forward-only** in bus order.

### 2) Mixer control semantics

Added:

- bus mute / solo
- group mute / solo

These are intentionally simple and host-friendly.

### 3) Sample-accurate automation queue

A new fixed-capacity queue allows sample-offset events to be applied inside the render loop.

Current targets:

- master gain
- headroom
- bus gain/pan
- group gain/pan
- voice gain/pan
- bus mute / solo
- group mute / solo
- bus send control

The queue is consumed without allocation and retains future events automatically.

### 4) Mix snapshots

`rm_mix_snapshot` captures mixer-state recall data for:

- master state
- limiter settings
- buses
- groups
- sends

The snapshot intentionally skips active voice state and internal filter memory.

### 5) Metering

Added bus and master meters with:

- peak
- smoothed envelope
- clip count
- master gain reduction readout

This is meant to be host-side friendly rather than GUI-heavy.

### 6) Lookahead limiter

The limiter got a bounded lookahead mode:

- compile-time fixed delay line
- no heap
- latency reporting
- same public simple limiter API kept for compatibility

## Tradeoffs kept on purpose

### No arbitrary dynamic DSP graph

That would push against:

- no heap
- simple deterministic memory layout
- easy C89 host integration

The forward-send model is a compromise that still enables useful aux workflows.

### No plugin hosting / MIDI / timeline

Those were explicitly left out for this pass.

The goal here was to make the library the best **audio mixer core** it can be under the fixed-point / static-memory rule set.

## Suggested next passes

If phase 6 ever happens, the most natural next moves would be:

- more native dynamics processors
- offline stem/tap rendering helpers
- sidechain-aware dynamics using fixed routing IDs
- explicit master/bus control ramps for scene morphing
- larger snapshot scopes for FX parameters
