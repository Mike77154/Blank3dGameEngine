# gvoice89 v2.0

Heapless logical/physical voice manager and fixed-point stereo mixer for strict C89 game engines.

## Headline configuration

- **256 logical voices** recommended by default.
- **64 physical voices** recommended for ordinary gameplay.
- Up to **256 physical voices** supported by the compiled core.
- Caller-owned storage; no heap and no hidden audio buffers.
- 16 buses, 32 group rules, per-instance limits, capability masks and generation-safe handles.

A logical voice owns timing, priority and provider state. A physical voice is one of the currently audible voices that is actually mixed. Quiet or low-priority logical voices can remain virtual and later return to the physical set.

## Voice selection

The physical set is rebuilt at a configurable control interval. Ranking uses:

1. `NEVER_VIRTUAL` and `CRITICAL` flags;
2. minimum physical hold time;
3. effective priority, including bus bias;
4. estimated audibility, gain and bus gain;
5. incumbent hysteresis to prevent rapid physical/virtual flutter;
6. deterministic serial ordering.

Group reserves are selected first, then the remaining physical budget is filled globally.

## Voice stealing

Stealing occurs only when the logical pool, a group limit or an instance-key limit is full. Policies include priority+audibility, hybrid, quietest, oldest, newest, furthest and reject. Voices can be protected with `NEVER_STEAL` or a start protection window.

A stolen voice leaves a 32-frame tail in a dedicated tail list, so the new voice can reuse the slot without an abrupt discontinuity.

## Virtual behavior

| Mode | Behavior while virtual | Typical use |
|---|---|---|
| `CONTINUE` | Calls the provider and discards output | exact procedural loops |
| `ADVANCE` | Uses `advance_frames`; falls back to silent processing | one-shots and seekable synths |
| `PAUSE` | Freezes provider time | local mechanisms or paused loops |
| `RESTART` | Restarts when promoted | disposable ambience/details |
| `KILL` | Ends when it cannot remain physical | tiny Foley and debris |

`virtual_timeout_ms` prevents forgotten loops or paused virtual voices from living forever.

## Heavy provider pooling

`physical_state_changed(user, is_physical)` is called on promotion and demotion. A host can use this to attach one of a small number of expensive DSP/synth states only while a logical voice is physical. The 256 logical records can therefore remain lightweight while 32–64 heavy providers are shared.

## Batch starts

`gv89_begin_batch()` and `gv89_end_batch()` defer ranking until the whole burst has been committed. This is useful for shotgun pellets, explosions spawning debris, or a frame containing many networked shots.

## Memory measured on this build

- `sizeof(gv89_voice)`: **168 bytes** on the 64-bit build host.
- `sizeof(gv89_context)`: **1,760 bytes**.
- 256 logical slots plus context: **44,768 bytes (43.72 KiB)**.

Provider/synth state belongs to the caller and is not included in that number.

## Build

```sh
make test
./build/virtual_256_demo
```

The library core uses C89, integer/fixed-point arithmetic and no `stdio`, `stdlib`, `math`, threads or audio backend.
