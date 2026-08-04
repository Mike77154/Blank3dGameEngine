# shotpumpkin89 1.1

`shotpumpkin89` is a **sample-free pump-action shotgun Foley synthesizer**.
It turns the filtered-white-noise and clustered-envelope idea behind an
808-style clap into a short mechanical gesture:

1. rearward **pulling/opening** stroke;
2. short configurable reversal gap;
3. forward **pumping/closing** stroke.

It synthesizes only the action noise, never the muzzle blast.

## Version 1.1 changes

- six-band fixed-point crossover EQ;
- tiny 8.7-14.3 Hz rail-chirr LFO, depending on preset;
- an extra band-limited white-noise layer for action-bar chatter;
- mechanism-informed presets for the Remington 870, Mossberg 590,
  Winchester 1300, Ithaca 37 and Benelli Nova;
- legacy three-band EQ remains available by setting `eq6_enabled = 0`;
- demo expanded to ten presets;
- deterministic test covers every preset and both EQ paths.

## Constraints

- strict C89 source;
- integer and fixed-point processing only;
- no dynamic allocation or heap ownership;
- no floating-point processing;
- no recorded samples and no asset files;
- caller-owned state and fixed-capacity delay buffers;
- mono signed PCM16 output;
- designed for 8-48 kHz sample rates;
- CC0-1.0.

## Signal path

```text
xorshift white noise
   |-- HP + LP friction body -- clap-derived microbursts ---------|
   |-- HP + LP rail chirr -- tiny triangle LFO -------------------|-- 6-band EQ
   `-- high-pass endpoint impact --------------------------------|       |
                                                                    soft distortion
                                                                          |
                                                                        chorus
                                                                          |
                                                                 short-room reverb
                                                                          |
                                                                      DC blocker
```

The rail LFO never modulates the entire action. It only gives the bright
band-limited `chirr` layer a very small irregular-sounding motion so the
noise does not sit still like a static synthesizer hiss.

## Six EQ bands

Default crossover points:

```text
Band 1: below 160 Hz       receiver thump / structural weight
Band 2: 160-380 Hz         lower body
Band 3: 380-850 Hz         fore-end and cavity body
Band 4: 850-1900 Hz        bars, carrier and bolt body
Band 5: 1900-4300 Hz       extractor/lock detail
Band 6: above 4300 Hz      scrape, edge and short metal chirr
```

The implementation uses five parallel one-pole low-pass states. Adjacent
low-pass outputs are subtracted to form six bands, then each band receives
a Q8 gain. This is deliberately small, deterministic and C89-friendly.

## Quick use

```c
#include "shotpumpkin89.h"

static sp89_state pump;
static sp89_config pump_cfg;

void audio_init(void)
{
    sp89_config_preset(&pump_cfg, 44100U, SP89_PRESET_REALISTIC);
    sp89_init(&pump, &pump_cfg);
}

void rack_shotgun(void)
{
    sp89_trigger_cycle(&pump); /* pull -> gap -> pump */
}

signed short audio_tick(void)
{
    return sp89_process(&pump);
}
```

Separate animation notifies are supported:

```c
sp89_trigger_pull(&pump);
sp89_trigger_pump(&pump);
```

## Presets

```text
SP89_PRESET_REALISTIC
SP89_PRESET_HEAVY
SP89_PRESET_OILED
SP89_PRESET_WORN
SP89_PRESET_CINEMATIC
SP89_PRESET_REMINGTON_870
SP89_PRESET_MOSSBERG_590
SP89_PRESET_WINCHESTER_1300
SP89_PRESET_ITHACA_37
SP89_PRESET_BENELLI_NOVA
```

The named presets are acoustic design interpretations, not claims of
forensic identification or sample-perfect cloning. They use manufacturer
mechanism/material descriptions to vary timing, body, lock brightness,
rail chatter and room decay.

## Main controls

All linear gains use Q8 (`256 == 1.0`). Wet mixes, feedback and LFO depth
use Q15 (`32767 == 1.0`).

- `pull_ms`, `gap_ms`, `pump_ms`;
- `friction_gain_q8`, `impact_gain_q8`, `chirr_gain_q8`;
- `eq6_gain_q8[6]`, `eq6_cross_hz[5]`;
- `chirr_hp_hz`, `chirr_lp_hz`;
- `chirr_lfo_rate_millihz`, `chirr_lfo_depth_q15`;
- `drive_q8`;
- chorus mix/rate/base delay/depth;
- reverb mix/feedback;
- deterministic seed and timing variation.

## Demo order

The WAV begins each preset one second apart, starting at 0.5 s:

```text
0.5  REALISTIC
1.5  HEAVY
2.5  OILED
3.5  WORN
4.5  CINEMATIC
5.5  REMINGTON_870
6.5  MOSSBERG_590
7.5  WINCHESTER_1300
8.5  ITHACA_37
9.5  BENELLI_NOVA
```

## Build

```sh
make
./shotpumpkin89_test
./shotpumpkin89_demo
```

The strict build command is:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -O2
```
