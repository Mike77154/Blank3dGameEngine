# ImageSequencer89 API

Sequences own contiguous frame ranges. Finish adding frames to one sequence
before starting another. A frame request is opaque text and may be a logical
asset key or explicit path.

`is89_player_step()` uses integer milliseconds and Q16.16 speed. Negative speed
plays toward earlier frames. Playback is deterministic and heap-free.
