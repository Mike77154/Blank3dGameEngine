# GoldieAudio89 + Monika FontCore vendor integration — Blank3D v3.27.7

## Goal

Blank3D now owns two engine-level provider boundaries while preserving the complete uploaded vendor source trees:

- `vendor/audio/goldie_audio89/`
- `vendor/text/monika_fontcore/`

The game-side code is deliberately small. Goldie owns mixing/console semantics; FontCore owns font decoding, measurement and glyph rasterization. Blank3D only adapts those capabilities to the existing runtime.

## Audio authority

The current host output remains the existing WinMM queue. Goldie runs with the KNM NULL backend as an offline/block mixer, so Blank3D does not start two competing hardware writers.

```text
weapon_synth_sound_engine89
        |
        | generic PCM callback
        v
GoldieAudio89 Matryoshka graph
MASTER
|- Dialogue
|- SFX
|  `- Weapons
|- UI
|- Ambience
|  |- Air
|  `- Biofauna
`- Music
   |- Rhythm
   `- Harmony
        |
        v
Goldie master PCM
        |
        v
Blank3D host WinMM queue
```

### Important boundary

`blank3d_goldie_audio89` does not include or know the weapon synthesizer API. It accepts a generic render callback:

```c
typedef int (*Blank3DGoldieAudioRenderFn89)(void *user,
                                             short *dst_interleaved,
                                             unsigned int frames);
```

`blank3d_audio.c` is the adapter that calls `wsse89_render_stereo`. This keeps Goldie reusable for any future PCM generator.

### Public host operations

- set gain on any named Blank3D bus
- mute/unmute any named bus
- play caller-owned PCM on a selected bus
- decode WAV into caller-owned static workspaces
- decode MP3 on Win32 through Goldie's ACM provider into caller-owned PCM

Decoded PCM must remain alive while a Goldie voice is using it. Blank3D does not allocate or copy it behind the caller's back.

## Text authority

`Blank3DText89` is an engine-level subsystem, not a field owned by BVHUD. The engine creates one text provider and injects its pointer into the HUD. Other systems can use the same provider later without depending on HUD code.

```text
project font asset
      |
      v
Blank3DText89 host adapter
      |
      v
Monika FontCore
  decode / UTF-8 measure / TTF glyph raster
      |
      +--------------------+
      |                    |
      v                    v
    BVHUD              future UI
 counters          menus/subtitles/etc.
```

### Static storage

The adapter embeds caller-owned/static work areas for:

- source font bytes
- reconstructed SFNT bytes
- decoder work bytes
- outline points/contours
- glyph atlas
- UTF-8 layout scratch
- grayscale text bitmap
- RGBA upload bitmap

No heap API was added to the host bridge.

### Fallback behavior

`config/blank3d.toml` now contains:

```toml
[text]
enabled = true
font = ""
pixel_size = 7
```

No font file is bundled. When `text.font` is blank, missing, unsupported for the current HUD raster path, or fails to decode, BVHUD keeps the existing 5x7 emergency/debug font. This makes startup deterministic and preserves old recipes.

When a project supplies a TTF/glyf font, HUD counters use FontCore measurement and antialiased RGBA raster output automatically.

FontCore keeps its full decoder facade (SFNT/TTF, OTF/CFF/CFF2, WOFF1, WOFF2, EOT scan, GameMaker sprite-font, MUGEN and Giffy HB). The current OpenGL HUD raster adapter is intentionally implemented first for FontCore's TTF/glyf raster path; other decoded formats remain available at the FontCore boundary for future raster providers.

## Build integration

The root Makefile now includes the complete Goldie runtime source set needed by Blank3D plus the complete Monika FontCore source set used by its own Makefile. Win32 also links `msacm32` for Goldie's MP3 ACM decoder.

FontCore is compiled with:

```text
-DGWT_USE_LONG_LONG=0
```

## Validation performed

- Monika FontCore vendor proof builds successfully.
- GoldieAudio89 vendor Linux smoke tests pass: NULL backend, WAV decode, Matryoshka mix.
- `blank3d_text89.c` passes strict C89/pedantic/Werror syntax checking.
- `blank3d_goldie_audio89.c` passes strict C89/pedantic/Werror syntax checking.
- Blank3D config changes pass strict C89/pedantic/Werror syntax checking.
- Goldie Blank3D host integration test passes with 11 buses and real Weapons -> SFX -> MASTER signal flow; muting Weapons reaches zero after the expected 32-frame MASTER limiter lookahead tail.
- FontCore Blank3D host preview renders real measured/rasterized HUD text successfully.
- Root Makefile release graph parses successfully in a dry run.
- New host bridges contain no `malloc/calloc/realloc/free`, `float/double`, `long long`, or explicit 64-bit integer token.
