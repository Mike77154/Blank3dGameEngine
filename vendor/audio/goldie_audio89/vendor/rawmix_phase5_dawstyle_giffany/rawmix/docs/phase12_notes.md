# rawmix phase 1 + 2 notes

This branch/revision implements the first two expansion phases discussed for the library, with a bias toward **keeping the original core deterministic and heap-free**.

## Added in phase 1

- priority-aware voice stealing
- protected voices
- headroom control
- clipping counter
- broader smoke coverage

### Voice stealing rule

The allocator now picks victims in this order:

1. free slot
2. lowest-priority **unprotected** voice
3. quieter voice on ties
4. oldest voice on ties

If every live voice is protected, a non-protected newcomer returns `RM_ERR_NO_FREE_VOICE`.

Protected newcomers are allowed to displace protected voices only when there are no free or unprotected slots left.

## Added in phase 2

- fixed bus table (`RAWMIX_MAX_BUSES`)
- fixed group table (`RAWMIX_MAX_GROUPS`)
- group stop / gain / pan ramp control
- per-voice gain+pan ramps
- fade-in on start
- fade-out helper
- minimal synchronous stream-source API
- start-delay and start-offset support

## Important tradeoffs

### Stream API is intentionally small

`rm_engine_play_stream()` uses `rm_stream_next_proc`, a synchronous callback that supplies one source frame at a time.

That means:

- no heap
- no background threads
- no decoder worker queue
- deterministic pull model

But it also means:

- callback overhead is higher than a page-buffered streamer
- no built-in seek API
- no compressed format loaders yet
- no async decode

For the next phase, the natural upgrade path is to add a higher-level resource/decoder layer **outside** the fixed-point core while keeping this low-level stream hook intact.

### Groups are fixed IDs, not dynamically allocated handles

That was chosen to preserve the no-malloc rule and keep the API simple in C89. Group `0` is the default group, and buses/groups are always available up to the compile-time limits.

## Suggested next phase after this

- block/page-based streaming callback
- official miniaudio/SDL adapter as an out-of-core backend layer
- FX per bus
- limiter on master
- optional higher-quality resampler modes
