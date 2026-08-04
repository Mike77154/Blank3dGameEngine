# ggunmach89 1.0

`ggunmach89` is a fixed-point C89 synthesizer for the motor, rotor, cam,
bolt and feed-mechanism character of externally powered Gatling-style guns.

It is an audio library. It does not simulate projectiles, ballistics or
weapon control.

## Design goals

- Strict C89 source.
- Integer/fixed-point signal path.
- No `malloc`, `realloc`, `free`, heap ownership, `float` or `double`.
- Caller-owned state and fixed internal buffers.
- Deterministic output from a deterministic seed.
- Mono 16-bit PCM rendering.
- Sample rates from 8 kHz through 96 kHz.
- Six-band fixed-point equalizer.
- Piecewise soft distortion.
- Optional fixed-buffer multi-tap reverb.
- Independent rotor, cam-cycle and pitch-ripple clocks.
- Spin-up and spin-down with integer error-accumulated ramps.

## Included presets

| ID | Character | Nominal rate | Barrels |
|---|---|---:|---:|
| `GGM89_PRESET_GENERIC` | Neutral rotary mechanism | 2400 SPM | 6 |
| `GGM89_PRESET_M134D` | Compact electric drive | 3000 SPM | 6 |
| `GGM89_PRESET_M197` | Slower, separated mechanism pulses | 1500 SPM | 3 |
| `GGM89_PRESET_M61` | Fast Vulcan-style whine | 6000 SPM | 6 |
| `GGM89_PRESET_GAU8` | Heavy low-mid hydraulic character | 3900 SPM | 7 |
| `GGM89_PRESET_GAU22` | Compact four-barrel high-speed character | 3300 SPM | 4 |

The nominal rates and barrel counts are grounded in manufacturer or U.S.
Air Force material. The envelope times, oscillator harmonics, EQ gains,
distortion and reverb values are artistic synthesis parameters, not claimed
measurements of a particular real weapon.

## Signal path

```text
speed/inertia state
      |
      +--> rotor phase --> tremolo
      +--> cam phase   --> bolt/cam transient
      +--> pitch phase --> light vibrato/torque ripple
      |
sine body + saw drive + filtered noise + cam transient
      |
six-band split EQ
      |
soft distortion
      |
fixed multi-tap reverb
      |
16-bit mono PCM
```

The two physical rates are derived from the current shot-rate setting:

```text
cam cycles per second = shots_per_minute / 60
rotor revolutions per second =
    shots_per_minute / (60 * barrel_count)
```

The rotor clock drives low-frequency mechanical breathing. The cam clock
represents the repeated bolt/cam/follower cycle. Oscillator frequencies are
harmonics of the rotor rather than arbitrary unrelated notes.

## Quick use

```c
#include "ggunmach89.h"

static ggm89_state motor;
static ggm89_s16 audio[512];

int setup(void)
{
    ggm89_config config;

    ggm89_config_preset(&config, GGM89_PRESET_M134D);
    if (!ggm89_init(&motor, &config, 44100UL)) {
        return 0;
    }

    ggm89_start(&motor);
    ggm89_set_firing_load_q15(&motor, 28000);
    return 1;
}

void audio_callback(void)
{
    ggm89_render_mono(&motor, audio, 512U);
}
```

Use `ggm89_stop()` to begin the configured spin-down. Use
`ggm89_set_firing_load_q15()` to strengthen cam chatter and introduce a
small RPM load dip without adding gunshot explosions.

## Build

### GCC / MinGW

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
    -Iinclude src/ggunmach89.c tests/test_ggunmach89.c \
    -o test_ggunmach89
```

Or:

```sh
make test
make demo
```

### Windows batch

Run:

```bat
build_mingw32.bat
```

The batch file expects `gcc` from an MSYS2/MinGW installation on `PATH`.

## Demo audio

`ggunmach89_showcase.wav` is 20 seconds at 44.1 kHz, mono, 16-bit PCM.

Timeline:

```text
00:00-00:04  M134D
00:04-00:08  M197
00:08-00:12  M61
00:12-00:16  GAU-8
00:16-00:20  GAU-22
```

Each segment starts the motor, introduces firing load at one second, and
begins spin-down at 2.5 seconds.

## Fixed-point formats

- Audio and modulation gains: Q1.15.
- EQ and distortion gains: Q4.12.
- Normalized rotor speed: unsigned Q16.
- Oscillator phases: 0..65535 wrapping phase.
- Filter and mixing accumulators: signed `long`.

No `long long` is required.

## Memory

`ggm89_state` owns a fixed 4096-sample reverb buffer. Call
`ggm89_state_size_bytes()` on the target compiler for the exact ABI size.
The strict test build in this package reported 8512 bytes on its build host.

## Files

```text
include/ggunmach89.h        Public API and caller-owned state
src/ggunmach89.c            Synthesizer implementation
examples/render_showcase.c  WAV renderer
tests/test_ggunmach89.c     Preset and state-transition tests
docs/API.md                 Function reference
docs/RESEARCH_NOTES.md      Physical/acoustic design mapping
ggunmach89_showcase.wav     Rendered demonstration
Makefile                    GCC/MinGW-compatible build
build_mingw32.bat           Windows build helper
LICENSE.txt                 CC0 dedication
```

## Scope

This first version synthesizes the machinery. It deliberately leaves muzzle
blast, individual shot explosion, projectile crack, casing ejection and
distant impact to separate libraries so a game engine can balance those
layers independently.

## License

CC0-1.0. See `LICENSE.txt`.
