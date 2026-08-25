# Goldie Audio System 89 v0.3.0 — True Matryoshka Submix Runtime

Goldie v0.3.0 changes the previous logical recursive bus tree into a real nested mixer graph.

Each `goldie_audio89_console` owns an independent vendored `rawmix` engine. A child console renders its local WAV/MP3/PCM voices and any deeper child consoles into a PCM block. Goldie injects that processed child output as one protected input voice into the parent console. The parent mixes/processes the combined signal and the graph is rendered bottom-up until `MASTER -> KNM_audio -> hardware`.

Example:

```
MASTER
├─ Dialogue
├─ SFX
├─ UI
├─ Ambience
│  ├─ Air
│  └─ Biofauna
│     └─ Birds
└─ Music
   ├─ Rhythm
   │  └─ Drums
   └─ Harmony
      └─ Synth
```

This is not parameter inheritance: `Birds` produces PCM, `Biofauna` mixes/processes that PCM, then `Ambience` receives the resulting PCM as a channel, and so on.

## Runtime properties

- C89 / fixed-point path.
- No `malloc`, `calloc`, `realloc` or `free` in Goldie runtime.
- Static configurable console pool (`GOLDIE_AUDIO89_MAX_CONSOLES`, default 64).
- Each console has its own rawmix voices, gain, pan, mute, FX chain, limiter, meters and stats.
- One parent per console; cycles rejected.
- Rendering order is precomputed bottom-up and uses no C recursion.
- Child console inputs and local source voices share `max_voices_per_console` rawmix input capacity.
- WAV via vendored `mwav89` + `mpcm89`.
- MP3 via vendored `mp3_frame89` + `mp3_acm_codec89` on Win32.
- Output via vendored KNM providers.

Run `./build_test_linux.sh` for strict C89 tests using the KNM null backend.
