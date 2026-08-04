# wsound_rocketblast89 v1.4.0

Procedural rocket-impact and launcher-explosion synthesis for real-time game audio.
The core is strict C89, fixed-point only, deterministic, and performs no dynamic allocation.

## Why v1.4 has more physical aftermath

Version 1.3 established the monumental direct/early/late architecture, but the post-impact body still leaned mainly on the central noise, low companion and tonal sub. Version 1.4 adds two genuinely independent stochastic layers without increasing heap use or introducing floating point:

- `WSRB89_NOISE_RUMBLE`: a low-passed pressure cloud with its own decay and low-pass SVF;
- `WSRB89_NOISE_CRACKLE`: sparse fracture impulses with their own decay and band-pass SVF.

A dedicated slow motion LFO modulates the rumble depth. The older chorus LFO remains restricted to the room send, so the direct pressure front stays centered and solid. The event still uses three routing fields:

```text
DIRECT IMPACT
  bipolar pressure front + darkened crack + turbulent body
  -> independent transient/body SVFs
  -> independent six-band EQ banks
  -> fixed-point limiter

EARLY FIELD
  five asymmetric reflections at roughly 12, 23, 37, 56 and 82 ms
  -> low-pass diffusion
  -> stereo placement without delaying the direct hit

LATE FIELD
  body/low-noise room cloud
  -> room-send-only chorus
  -> 3 combs + 2 allpasses per channel
  -> dark frequency-dependent decay
```

The direct impact therefore remains immediate and centered, while the reflections and low-frequency tail arrive behind it and make the event occupy a larger virtual space.

## Seven independent noise generators

1. crack / pressure-front texture
2. central turbulent body
3. sparse debris
4. independent low spectral companion
5. independent high spectral companion
6. long low-frequency rumble
7. sparse crackle/fracture tail

Noise has no literal musical octave. The low/high companion names refer to approximate spectral placement around the central body.

## Main v1.4 behavior

- near-discontinuous pressure onset followed by positive and negative phases
- body EQ emphasizes 40-180 Hz weight without turning the sub into a sustained note
- transient path now passes through its own SVF and six-band EQ
- strong cuts in the 3.2 kHz and 7.2 kHz bands remove comic-book fizz
- five-tap early-reflection network supplies immediate scale and stereo width
- reverb expanded from two combs/one allpass to three combs/two allpasses per channel
- chorus moved entirely to the room send so the direct pressure front stays solid
- stochastic low-frequency room cloud prevents a clean sine tail from becoming audible
- dedicated rumble noise adds non-tonal low-frequency aftermath
- dedicated crackle noise adds irregular debris/fracture detail after the pressure front
- independent rumble/crackle SVFs prevent either layer from contaminating the full spectrum
- dedicated motion LFO subtly breathes the rumble; the chorus LFO remains room-send-only
- longer decay/release presets with darker late energy
- signed SVF state clamping prevents fixed-point multiplication overflow
- DC blocker settles to exact zero after the tail
- distortion drive is unchanged or lower than v1.2 presets

## Constraints

- ISO C89
- no dynamic allocation
- no floating-point types or operations
- no `math.h`
- caller-owned static workspace
- Q15, Q14, Q12, and Q8 arithmetic
- 8,000 to 48,000 Hz
- signed 16-bit planar stereo output

The core and public header include no C library headers. `stdio.h` appears only in the WAV demo and test reporter.

## Memory

- workspace: **48,004 bytes**
- context: **500 bytes** in the tested i386 object ABI
- context: **504 bytes** in the tested 64-bit executable ABI
- fully independent i386 voice: **48,504 bytes** (about 47.4 KiB)

The workspace remains unchanged from v1.3; the context grows modestly for two envelopes, two SVFs, two RNG streams and the motion LFO. The original increase from v1.2 paid for the five early-reflection taps, an extra comb and allpass stage per channel, and the second EQ/SVF state bank.

## Presets

| ID | Preset | Character |
|---:|---|---|
| 0 | `WSRB89_PRESET_HEAVY_IMPACT` | monumental general-purpose rocket impact |
| 1 | `WSRB89_PRESET_CONCRETE_PAAS` | hard pressure hit with concrete debris |
| 2 | `WSRB89_PRESET_METAL_STRIKE` | dense metallic bite without excessive fizz |
| 3 | `WSRB89_PRESET_AIRBURST` | fast open-air transient with a shorter field |
| 4 | `WSRB89_PRESET_INDOOR_BUNKER` | dark enclosed pressure and the longest room tail |
| 5 | `WSRB89_PRESET_DISTANT_PAAS` | low-frequency distant event with broad reflections |
| 6 | `WSRB89_PRESET_COMPACT_RPG` | shorter close-range launcher impact |

## Minimal integration

```c
#include "wsound_rocketblast89.h"

static wsrb89_context rocket_voice;
static wsrb89_workspace rocket_memory;
static wsrb89_s16 left_buffer[512];
static wsrb89_s16 right_buffer[512];

void rocket_audio_init(void)
{
    wsrb89_params p;

    wsrb89_init(&rocket_voice, &rocket_memory, 44100U, 0x12345678U);
    wsrb89_get_preset(&p, WSRB89_PRESET_HEAVY_IMPACT);
    wsrb89_set_params(&rocket_voice, &p);
}

void rocket_impact(void)
{
    wsrb89_trigger(&rocket_voice, 32767U);
}

void rocket_audio_render(void)
{
    wsrb89_render_stereo(&rocket_voice,
                         left_buffer,
                         right_buffer,
                         512U);
}
```

Keep rendering while `wsrb89_is_active()` is nonzero so the early and late room fields can finish.

## Parameter formats

- amplitudes and wet/dry values: Q15
- EQ contributions: signed Q14
- drive/output gain: Q12
- sub frequencies: unsigned Q8 Hz
- chorus rate: millihertz
- time values: milliseconds

The six EQ centers are approximately 80, 180, 450, 1,200, 3,200, and 7,200 Hz.

## Build

```sh
make
make preview
make smoke
```

For MinGW32:

```bat
build_mingw32.bat
```

All standard previews are rendered by `demo/wsrb89_demo.c` through the same fixed-point core. The release contains `wsrb89_ab_5noise_vs_7noise_rumble_crackle.wav` and `wsrb89_rumble_crackle_layers.wav` for direct comparison and isolated-layer audition.

## Files

```text
include/wsound_rocketblast89.h   public API and static buffers
src/wsound_rocketblast89.c       voice, routing, envelopes and room field
src/wsrb89_dsp.c                 integer DSP helpers
src/wsrb89_presets.c             seven tuned presets
demo/wsrb89_demo.c               WAV renderer
tests/wsrb89_smoke.c             deterministic multi-rate tests
docs/PHYSICS_NOTES.md            physics-to-synthesis mapping
docs/TUNING.md                   parameter guide
docs/VALIDATION.md               build and output checks
```

The previews contain sharp transients. Audition them at low volume first.
