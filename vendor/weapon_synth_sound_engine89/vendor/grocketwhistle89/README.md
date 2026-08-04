# grocketwhistle89 v1.2

Sustained rocket and missile trajectory-whistle synthesizer for game engines.
The complete DSP path uses integer fixed-point C89, caller-owned state and no
runtime allocation.

Version 1.2 keeps the rocket plume as the physical priority, but borrows the
airy, unstable and spatial character of pyrotechnic whistles so the result is
less like a clean electronic sine oscillator.

## Signal path

```text
trajectory macro pitch + Doppler
        |
        +-- sine fundamental
        +-- weak 2nd/3rd harmonics
        +-- smoothed pitch/amplitude roughness
        +-- delayed band-limited plume air
        +-- phase-related aerated tone
                    |
              drive / soft clip
                    |
        chorus + separate stereo air delays
                    |
          primary six-band contour EQ
                    |
          optional short damped reverb
                    |
       secondary six-band finishing EQ
                    |
              PCM16 stereo / mono
```

## Why it is less electronic

A perfectly stable sine with a perfectly repeated vibrato reads as a synth
whistle. Version 1.2 adds four bounded imperfections:

- low-level second and third harmonics;
- slowly smoothed random pitch movement;
- slowly smoothed amplitude movement;
- a noise layer whose level follows the instantaneous whistle cycle.

The noise remains band-limited and the perturbations are deliberately small,
so the projectile still reads as a rocket or missile rather than a firework.

## Dual six-band equalization

### Primary contour EQ

This EQ sits after distortion and stereo spreading but before reverb. It shapes
the source character.

| Band | Range | Intended control |
|---|---:|---|
| 0 | below 180 Hz | sub cleanup |
| 1 | 180-420 Hz | plume body |
| 2 | 420-900 Hz | low whistle / heavy rocket mass |
| 3 | 900-1800 Hz | main missile whistle |
| 4 | 1800-3600 Hz | nozzle edge and harmonics |
| 5 | above 3600 Hz | air and distance texture |

### Secondary finishing EQ

The new EQ is placed after the optional reverb, so it also trims low-frequency
energy introduced by the tail and raises the final airborne edge.

| Band | Range | Intended control |
|---|---:|---|
| 0 | below 120 Hz | final rumble rejection |
| 1 | 120-300 Hz | final low-body reduction |
| 2 | 300-900 Hz | core weight |
| 3 | 900-2400 Hz | whistle intelligibility |
| 4 | 2400-6000 Hz | airy bite |
| 5 | above 6000 Hz | spatial hiss / distance |

Both EQs reconstruct six contiguous bands from five integer one-pole crossover
states. `32767` is unity Q15 gain; `65535` is approximately 2x.

```c
gwh89_set_output_eq_band_gain_q15(
    &rocket,
    GWH89_OUTPUT_EQ_RUMBLE_120,
    1800
);
gwh89_set_output_eq_band_gain_q15(
    &rocket,
    GWH89_OUTPUT_EQ_EDGE_2400_6000,
    40000
);
gwh89_set_output_eq_band_gain_q15(
    &rocket,
    GWH89_OUTPUT_EQ_AIR_ABOVE_6000,
    38000
);
```

Use `gwh89_set_eq_flat()` and `gwh89_set_output_eq_flat()` to flatten either
stage independently. Both preset shapes are restored when a preset is
triggered.

## Air-space layer

A second fixed delay line carries only the filtered plume-air branch. Left and
right use different short delays plus a tiny shared modulation. This avoids the
static centered hiss of a basic synth and suggests an extended moving source
without requiring convolution, FFTs or heap memory.

## Optional reverb

The reverb remains a restrained three-comb damped texture. It suggests nearby
terrain and open-air reflections rather than a room. The final EQ follows it,
so low rumble can be removed even when the tail is enabled.

```c
gwh89_set_reverb(
    &rocket,
    1,      /* enabled */
    2600,   /* wet Q15 */
    17400,  /* feedback Q15 */
    7600,   /* damping Q15 */
    220     /* tail milliseconds */
);
```

## Minimal use

```c
#include "grocketwhistle89.h"

static gwh89_state rocket;
static gwh89_s16 stereo_block[512 * 2];

void start_rocket(void)
{
    gwh89_init(&rocket, 44100, 0x12345678U);
    gwh89_trigger_preset(&rocket, GWH89_PRESET_RPG7_SUSTAINED);
    gwh89_set_auto_hold_ms(&rocket, 0);
}

void audio_tick(void)
{
    /* Negative velocity approaches; positive velocity recedes. */
    gwh89_set_motion(&rocket, -65, 30000);
    gwh89_render_stereo(&rocket, stereo_block, 512);
}

void rocket_destroyed(void)
{
    gwh89_release(&rocket);
}
```

## Presets

Six weapon-oriented presets remain the primary set:

- RPG-7 sustained airy trajectory
- SMAW short airy rocket
- Javelin soft two-stage airy trajectory
- AT4 fast airy flight
- guided missile airy flyby
- heavy rocket airy low whistle

A seventh diagnostic preset, `GWH89_PRESET_CHIFLADORA_AIR_REF`, exaggerates the
high harmonics, instability and stereo air inspired by Mexican pyrotechnic
"chifladores". It is a sound-design reference, not a forensic model and not the
default for missile simulation.

## Build

```sh
make
make test
make previews
```

Strict validation command:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2 \
    grocketwhistle89.c test_api.c -o test_api
```

A MinGW batch file is included for Windows.

## Constraints

- ISO C89 source style
- signed 16-bit PCM output
- 32-bit integer fixed-point processing
- no `malloc`, `realloc`, `free`, `float` or `double` in the core
- no core dependency on stdio, stdlib or math
- no global mutable DSP state
- caller-owned state and fixed internal buffers
- deterministic seeded noise

## Memory

`sizeof(gwh89_state)` is 18,296 bytes with the validated GCC build. The new air
space delay requires 512 bytes; the remainder of the increase is phase,
roughness and second-EQ state.

See `SOURCES.md`, `CHANGELOG.md` and `VALIDATION.txt` for research translation,
changes and test results.
