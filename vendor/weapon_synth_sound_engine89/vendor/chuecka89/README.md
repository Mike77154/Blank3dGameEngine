# chuecka89 v2.4 — physics/reference pass

This pass keeps the approved **SH attack 30 ms / SH release 12 ms** atom, dries its loose air, and rebuilds weapon presets around documented mechanical stages: friction/slide, extraction or indexing, feed/chamber, and final lock/contact.


## v2.4: átomo aprobado (ataque 30 ms, release 12 ms) y presets reconstruidos con FX

- El ataque de la voz `SH` del átomo queda en 30 ms.
- La cola `release` de la voz `SH` permanece en 12 ms.
- Se añadió una máscara de efectos por gesto para aislar distorsión, EQ, chorus y reverb.
- Incluye render por etapas con la misma semilla de ruido y montajes igualados por pico.


`chuecka89` is a compact procedural Foley library for weapon-handling sounds. The v2 sound atom follows this sequence:

```text
SH                         ECKT
noise with audible attack  noise with zero attack
friction / scrape           short decay / mechanical closure
          \_____________________/
                    SHEKT
```

The two voices are sequential, not merely mixed at the same instant. Each has its own envelope, distortion, six-band EQ profile, EQ sweep, noise color, and level. Chorus and a very small reverb are applied after the voices are combined.

## Included sounds

- isolated `SH`
- isolated `ECKT`
- combined `SHEKT`
- shotgun pump `chik-chok`
- shotshell insertion `shuk`
- repeated shotshell loading `sheke-sheke-sheke`
- pistol slide
- heavy semiautomatic action
- revolver cylinder indexing
- grenade-launcher breech handling
- rocket-launcher handling
- SMG cyclic feed texture
- machine-gun cyclic feed texture

The library renders handling Foley only. It does not generate muzzle blast, projectile flight, impact, or detonation.

## Constraints

- ISO C89 core
- Q15 fixed-point signal path
- no dynamic storage
- caller-owned/static buffers
- no external DSP dependency
- 44,100 Hz and 48,000 Hz
- mono signed PCM16 output
- deterministic seeded noise
- CC0-1.0

## v2 signal path

```text
SH white noise
  -> SH envelope
  -> SH distortion
  -> SH six-band moving EQ
                           \
                            + -> chorus -> tiny reverb -> output gain
                           /
ECKT white noise
  -> delayed zero-attack envelope
  -> ECKT distortion
  -> ECKT six-band moving EQ
```

The separate EQ states are important. The `SH` voice can remain airy and friction-like while `ECKT` collapses rapidly into a low-mid metallic stop. The EQ motion is interpolation between two six-band gain sets; it does not require a pitched oscillator.

## Preset previews

```text
wav/10_atom_sh.wav
wav/11_atom_eckt.wav
wav/12_atom_shekt.wav
wav/00_shotgun_pump_chik_chok.wav
wav/01_shotgun_insert_shuk.wav
wav/02_shotgun_multi_insert_sheke.wav
wav/97_v1_then_v2_shotgun_comparison.wav
wav/98_focus_atom_and_shotgun.wav
wav/99_all_presets_montage.wav
```

Comparison order in `97_v1_then_v2_shotgun_comparison.wav`:

```text
v1 pump -> v2 pump
v1 insert -> v2 insert
v1 repeated insert -> v2 repeated insert
```

Focus montage order:

```text
SH -> ECKT -> SHEKT -> shotgun pump -> shell insert -> repeated insert
```

## Audición de efectos del átomo

```text
wav/20_atom_fx_00_dry.wav
wav/21_atom_fx_01_distortion.wav
wav/22_atom_fx_02_distortion_eq.wav
wav/23_atom_fx_03_distortion_eq_chorus.wav
wav/24_atom_fx_04_full_subtle.wav
wav/25_atom_fx_stage_montage.wav
wav/29_atom_fx_strength_montage.wav
wav/96_atom_dry_then_full_fx.wav
wav/97_shotgun_presets_full_fx_montage.wav
wav/98_final_atom_then_shotguns.wav
wav/99_all_weapon_presets_full_fx_montage.wav
```

Los WAV de armas se regeneraron desde los presets del código con la cadena completa activa; no se reutilizaron renders anteriores.

```text
```

La máscara puede cambiarse sin reconstruir el preset:

```c
ch89_set_effect_flags(&gesture, CH89_EFFECT_ALL);
```

El orden es distorsión por voz, EQ móvil de seis bandas, chorus global y reverb global diminuta. Consulta `docs/ATOM_FX_AUDITION.md`.

## Build

```sh
make
make test
make preview
```

Optional 32-bit i386 core object and archive on GCC installations that support `-m32`:

```sh
make core32
```

Default validation flags:

```text
-O2 -Wall -Wextra -pedantic -std=c89
```

## Core use

```c
#include "chuecka89.h"

static ch89_context ctx;
static ch89_gesture gesture;
static ch89_i16 output[48000];
static ch89_u32 written;

ch89_init(&ctx, 48000U, 12345U);
ch89_make_preset(
    CH89_PRESET_SHOTGUN_PUMP,
    0U,
    CH89_Q15_ONE,
    &gesture
);
ch89_render(&ctx, &gesture, output, 48000U, &written);
```

## Editing the base atom

```c
/* Friction voice. */
gesture.strokes[0].sh.envelope.attack_ms = 30U;
gesture.strokes[0].sh.envelope.decay_ms = 62U;
gesture.strokes[0].sh.envelope.release_ms = 12U;

/* Closure voice: starts after SH and attacks immediately. */
gesture.strokes[0].eckt.envelope.delay_ms = 82U;
gesture.strokes[0].eckt.envelope.attack_ms = 0U;
gesture.strokes[0].eckt.envelope.decay_ms = 15U;
```

Each voice exposes:

```text
envelope.delay_ms
envelope.attack_ms
envelope.decay_ms
envelope.sustain_ms
envelope.release_ms
envelope.sustain_q15
envelope.level_q15
envelope.color
eq_start_q15[6]
eq_end_q15[6]
eq_sweep_ms
distortion_q15
```

## v1 migration

The v2 stroke layout is intentionally more explicit and changes the source ABI:

```text
v1 stroke.scrape             -> v2 stroke.sh.envelope
v1 stroke.stop               -> v2 stroke.eckt.envelope
v1 stroke.eq_q15             -> independent SH/ECKT start and end EQ arrays
v1 stroke.distortion_q15     -> independent SH/ECKT distortion values
```

## Memory model

`ch89_context` owns fixed chorus/reverb delay lines and independent EQ/noise states for both voices of every stroke. One context renders one stream at a time. For concurrent real-time voices, provide one static context per active stream or render one-shots into engine-owned buffers.

## License

CC0-1.0. Vendor it, fork it, rename it, or ship it inside an engine.

## v2.4 physics-pass audition

```text
wav/96_atom_v23_then_v24_physics.wav
wav/97_shotgun_v23_then_v24_physics.wav
wav/98_physics_pass_focus.wav
wav/99_all_presets_physics_pass.wav
```

`96` plays v2.3 then v2.4. `97` compares old/new shotgun pump, insertion, and repeated loading. `98` is a focused tour of the rebuilt mechanisms. See `docs/PHYSICS_PASS.md`.
