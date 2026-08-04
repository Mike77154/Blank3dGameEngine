# gpaah89 BASE PRESETS v5.0

Deterministic procedural gunshot-burst synthesizer for game engines. This is
the approved baseline preset collection after the HEAVY HIT / OVERPRESSURE
calibration passes.

## Exact signal path

```text
3 pure-noise oscillators
  -> transient correlation of those same streams
  -> independent no-attack decay/sustain/release envelopes
  -> 6 non-resonant band-pass EQ bands, including mids
  -> envelope-driven parallel distortion
  -> feed-forward stereo chorus
  -> 6-tap feed-forward FIR reverb
  -> signed 16-bit PCM
```

No samples, pitched oscillators, resonators, Q filters, feedback effects,
mechanical layers, projectile cracks, compressors, FFT stages, external DSP
libraries or hidden allocations are used.

## Final base presets

- `pistol`: HEAVY HIT v4 low/low-mid body and natural tail;
- `magnum`: OVERPRESSURE heavy revolver profile;
- `shotgun`: **BASE v5 deeper tuning**, focused toward 25-600 Hz with a longer
  low-pressure body and reduced upper dryness;
- `metralla`: HEAVY HIT v4 readable burst with overlapping low body;
- `gatling`: HEAVY HIT v4, 20 ms demo retrigger and continuous pressure tunnel;
- `rocket_launcher`: **BASE v5 deeper tuning**, focused toward 20-500 Hz with a
  longer dark expansion and less upper-band rasp;
- `sniper_rifle`: OVERPRESSURE hard front and long rifle body.

Only shotgun and rocket-launcher DSP parameters changed from HEAVY HIT v4.

## Restrictions

- strict C89 core;
- integer fixed-point only: Q15, Q14 and Q8;
- no `malloc`, `calloc`, `realloc` or `free`;
- no `float` or `double` in the DSP core;
- caller-owned fixed-size state;
- exactly three noise generators and six EQ bands;
- chorus and reverb have zero feedback.

## Build

```sh
make clean
make
make samples
make test
```

Default flags:

```text
-std=c89 -pedantic -Wall -Wextra -Werror -O2
```

## Minimal integration

```c
#include "gpaah89.h"

static gpaah89_state gun;
static gpaah89_preset preset;
static gpaah89_s16 stereo_block[512 * 2];

void audio_init(void)
{
    gpaah89_get_preset(GPAAH89_PRESET_SHOTGUN, &preset);
    gpaah89_init(&gun, 44100U, &preset, 12345U);
}

void fire(void)
{
    gpaah89_trigger(&gun, 67890U);
}

void audio_callback(void)
{
    gpaah89_render_stereo(&gun, stereo_block, 512U);
}
```

## Listening files

- `samples/base_presets/`: all approved baseline renders;
- `samples/ab_comparisons/`: HEAVY HIT v4, silence, then BASE v5 for shotgun
  and rocket launcher;
- `samples/gpaah89_base_presets_showcase.wav`: full preset showcase.

The previews are normalized game audio, not calibrated firearm pressure.
Begin playback at moderate volume.

## License

CC0-1.0. See `LICENSE`.
