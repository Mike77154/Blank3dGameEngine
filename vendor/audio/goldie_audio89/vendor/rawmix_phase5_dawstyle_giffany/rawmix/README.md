# rawmix

`rawmix` is a **C89**, **fixed-point**, **no-heap** PCM mixer core for realtime and offline host-driven audio.

This snapshot is effectively a **phase 5 / DAW-style mixer pass** built on the earlier phase 1-4 core.

## Core rules

- no `malloc`, `calloc`, `realloc`, `free` in the library core
- no `float` / `double` in the library core
- fixed compile-time storage
- user-owned buffers and streams
- C89 public API
- mono/stereo signed 16-bit PCM
- deterministic one-pass processing

## What is in this snapshot

### Existing core

- static voice pool
- buffer voices and pull-stream voices
- looped and one-shot playback
- fixed-point gain / pan / pitch / ramps
- HQ cubic resampling for buffers and streams
- full-duplex host processing + capture ring
- buses / groups / per-bus FX inserts
- raw biquad inserts
- soft limiter
- priority / protected voices

### New DAW-style mixer features

- **bus sends / aux routing** with fixed storage
- **pre-fader and post-fader sends**
- **bus mute / solo**
- **group mute / solo**
- **sample-accurate automation queue** with fixed capacity
- **mix snapshots** for bus / group / master / limiter / sends
- **bus and master meters**
- **lookahead master limiter** with compile-time bounded delay line
- **latency reporting** for the limiter lookahead path
- updated smoke tests and a new showcase example

## Important design note about routing

To keep the engine:

- one-pass
- deterministic
- heap-free
- C89-simple

bus sends are intentionally **forward-only** in the current implementation.

That means:

```text
bus 0 -> bus 1   OK
bus 1 -> bus 3   OK
bus 3 -> bus 1   not supported in the same pass
bus 2 -> bus 2   not supported
```

This keeps aux routing useful without turning the engine into a dynamic graph allocator.

## Sample-accurate automation model

Automation is queued into a fixed array inside `rm_engine`.

Each event has:

- a `sample_offset` relative to the next render horizon
- a target type
- target IDs / handle
- values

Supported automation targets include:

- master gain
- headroom
- bus gain/pan
- group gain/pan
- voice gain/pan
- bus mute / solo
- group mute / solo
- bus send level / mode

Events scheduled beyond the current render call are retained and shifted down automatically after rendering.

## Snapshot model

`rm_mix_snapshot` captures:

- master gain / headroom / monitor
- limiter settings
- bus gain / pan / mute / solo
- group gain / pan / mute / solo
- send matrix state

It intentionally does **not** capture active voices or internal filter history.

## Metering model

Meters expose a lightweight fixed-point view of the last render block:

- peak left / right
- smoothed envelope left / right
- gain reduction (master limiter)
- clip events

## Limiter model

The old limiter path is still available through:

```c
rm_engine_set_limiter(...)
```

The new extended path is:

```c
rm_engine_set_limiter_ex(..., lookahead_frames)
```

Notes:

- lookahead is bounded by `RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES`
- latency is reported through `rm_engine_get_latency_frames()`
- the limiter remains intentionally lightweight and fixed-point
- this is still a **mix safety / control limiter**, not a mastering suite

## Public API additions

### Bus sends / mute / solo

```c
rm_engine_set_bus_mute(&engine, bus_id, 1U);
rm_engine_set_bus_solo(&engine, bus_id, 1U);
rm_engine_set_bus_send(&engine, 0U, 1U, RM_Q15_HALF, RM_SEND_PRE_FADER);
rm_engine_clear_bus_send(&engine, 0U, 1U);
```

### Group mute / solo

```c
rm_engine_set_group_mute(&engine, group_id, 1U);
rm_engine_set_group_solo(&engine, group_id, 1U);
```

### Automation

```c
rm_automation_event ev;
memset(&ev, 0, sizeof(ev));
ev.sample_offset = 128U;
ev.type = (rm_u8)RM_AUTOMATION_BUS_SET;
ev.target_id = 1U;
ev.value0_q15 = RM_Q15_ONE;
ev.value1_q15 = RM_PAN_LEFT;

rm_engine_queue_automation(&engine, &ev);
```

### Snapshots

```c
rm_mix_snapshot snapshot;
rm_engine_capture_snapshot(&engine, &snapshot);
rm_engine_apply_snapshot(&engine, &snapshot, 0U);
```

### Metering

```c
rm_meter_state meter;
rm_engine_get_bus_meter(&engine, 1U, &meter);
rm_engine_get_master_meter(&engine, &meter);
```

### Lookahead limiter

```c
rm_engine_set_limiter_ex(&engine,
                         (rm_s16)26000,
                         2U,
                         64U,
                         RM_Q15_ONE,
                         32U);
```

## Compile-time knobs

```text
RAWMIX_MAX_VOICES
RAWMIX_CAPTURE_RING_FRAMES
RAWMIX_MAX_BUSES
RAWMIX_MAX_GROUPS
RAWMIX_MAX_BUS_FX
RAWMIX_MAX_AUTOMATION_EVENTS
RAWMIX_LIMITER_MAX_LOOKAHEAD_FRAMES
```

## Build

```sh
make
make test
make demos
```

Artifacts now include:

- `build/offline_mix.wav`
- `build/host_duplex_demo.wav`
- `build/host_duplex_capture.wav`
- `build/daw_style_showcase.wav`

## Examples

- `examples/offline_mix.c` – original offline render demo
- `examples/host_duplex_demo.c` – duplex/capture demo
- `examples/daw_style_showcase.c` – sends + automation + limiter lookahead demo

## Current boundaries

This is much closer to a **mixer engine** than before, but it is still intentionally **not**:

- a MIDI engine
- a plugin host
- a heap-backed arbitrary graph system
- a full DAW session/timeline editor

The current sweet spot is:

```text
fixed-point mixer core
+ aux/submix routing
+ automation inside the render path
+ snapshot recall
+ lightweight metering
+ bounded-latency limiter
```

For the rules this project follows, that is already a pretty aggressive jump in capability.
