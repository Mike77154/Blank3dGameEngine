# Integration

```c
#define VOICES 256
static gv89_voice slots[VOICES];
static gv89_context voices;

gv89_init(&voices, slots, VOICES, 44100U);
gv89_set_physical_limit(&voices, 64U);
```

For a burst, call `gv89_begin_batch`, reserve/commit or start every voice, then call `gv89_end_batch`. Render with `gv89_render_stereo` or pull one sample with `gv89_process_stereo_sample`.

Use `gv89_set_voice_audibility` whenever distance, occlusion or focus changes. The value is Q15: 32767 means fully audible and 0 means inaudible.

The manager is single-threaded by design. Put command production on another thread only if the host supplies its own lock-free command queue and applies commands on the audio thread.
