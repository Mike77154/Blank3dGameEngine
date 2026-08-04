# wmagazine89

Procedural synthesis for firearm magazine handling sounds: insertion, removal,
seating taps, tug checks, and internal rattles.

The library is designed as a small mechanical Foley synthesizer rather than a
sample player. Its base exciter is a deliberately simple TR-style clap model:
white noise, a mid-focused filter, several short one-shot pulse envelopes, and a
short noise tail. That exciter drives friction, catch, spring, thump, and modal
body layers so the result can become `crk-chech`, `crrr-raaak`, `thok`, or a
small metallic `tchk` instead of remaining an electronic hand clap.

## Hard constraints

- strict C89 core
- integer and Q15 fixed-point processing only
- no `malloc`, `realloc`, `free`, or heap ownership
- no `float` or `double`
- no `math.h`, `stdlib.h`, `stdio.h`, or `string.h` in the core
- fixed-capacity arrays
- all mutable DSP state supplied by the caller
- no global mutable buffers
- mono signed PCM output, one sample or one block at a time
- sample rates from 8 kHz through 48 kHz

The optional WAV demo uses `stdio.h`; the synthesizer core does not.

## Signal path

```text
xorshift white noise
        |
        +--> four-pulse / scrape event sequencer
        |          |
        |          +--> low / mid / bright source coloration
        |
        +--> catch, spring and seating excitations
                   |
                   v
          four damped modal oscillators
                   |
                   v
          soft saturation / distortion
                   |
                   v
            six-band parallel EQ
                   |
                   v
         short triangle-LFO chorus
                   |
                   v
      three-comb + all-pass room reverb
                   |
                   v
                PCM16
```

## Why the clap core works

The TR-808 service notes describe its hand-clap source as white noise passed
through a band-pass filter and split across two envelope paths. Its main
sawtooth-like envelope produces a rapid cluster of pulses, while a second,
longer envelope produces the reverberant component. `wmagazine89` keeps only
the cheap and useful part of that topology:

```text
pulse 1       pulse 2       pulse 3                 catch
|             |             |                       |
vvvvvv        vvvvvvv       vvvvvvv                 vvvvvvvvv
0 ms          10 ms         20 ms                   ~35 ms
```

The pulse attacks are nearly vertical and their decays are short and linear.
The default broad mid band is intentional. It produces the compact `crk` body
before material resonances and effects are added.

## Mechanical model

Magazine handling is split into events because the mechanism is not one impact:

```text
magazine enters the well or guide rails
  -> sliding / rough contact
  -> small wall and lip collisions
  -> magazine catch engagement
  -> optional base strike
  -> optional downward tug check
  -> body, follower, cartridges and spring decay
```

The default AR-like insert ends in catch engagement. The SMG steel preset
stretches the scrape and emphasizes wall contact. The sniper box preset has
slower low modes. The drum preset emphasizes internal mass and rattle.

## Presets

```c
WMAG89_PRESET_PISTOL_METAL
WMAG89_PRESET_PISTOL_POLYMER
WMAG89_PRESET_SMG_STEEL
WMAG89_PRESET_RIFLE_ALUMINUM
WMAG89_PRESET_RIFLE_POLYMER
WMAG89_PRESET_SNIPER_BOX
WMAG89_PRESET_DRUM_HEAVY
```

## Built-in actions

```c
WMAG89_ACTION_INSERT
WMAG89_ACTION_REMOVE
WMAG89_ACTION_SEAT_TAP
WMAG89_ACTION_TUG_CHECK
WMAG89_ACTION_RATTLE
```

The action is a one-shot. Call `wmag89_trigger()` on an animation edge, not on
every frame while the animation remains in the same state.

## Minimal integration

```c
#include "wmagazine89.h"

static WMag89State magazine_sound;
static short mix_block[256];

void audio_init(void)
{
    wmag89_init(&magazine_sound, 44100, 0x12345678U);
    wmag89_set_preset(&magazine_sound, WMAG89_PRESET_RIFLE_POLYMER);
}

void animation_magazine_catch_edge(void)
{
    wmag89_trigger(&magazine_sound, WMAG89_ACTION_INSERT, 30000);
}

void audio_callback(void)
{
    wmag89_process_block(&magazine_sound, mix_block, 256);
    /* Mix mix_block into the engine output. */
}
```

## Custom crack sequencer

Up to twelve caller-authored events can be copied into the fixed state buffer.
Times are integers in milliseconds and gains are Q15.

```c
static const WMag89Event long_insert[] = {
    { 0,  7, 26000, WMAG89_EVENT_BURST, 1 },
    { 9,  8, 22000, WMAG89_EVENT_BURST, 1 },
    { 2, 48, 18000, WMAG89_EVENT_SCRAPE, 1 },
    { 52, 10, 31000, WMAG89_EVENT_CATCH, 2 },
    { 58, 20, 13500, WMAG89_EVENT_SPRING, 2 },
    { 50, 22, 12000, WMAG89_EVENT_THUMP, 0 }
};

wmag89_trigger_custom(
    &magazine_sound,
    long_insert,
    (int)(sizeof(long_insert) / sizeof(long_insert[0])),
    32767
);
```

Event types:

```text
BURST   short clap-like micro-impact
SCRAPE  triangularly pulsed, rough noise contact
CATCH   hard latch engagement and strong modal excitation
SPRING  bright internal movement
THUMP   low seating or base impact
```

Color values:

```text
0 = low/body
1 = broad mid contact
2 = bright lip/catch contact
```

## Six-band EQ

The EQ is a low-cost parallel filter bank with nominal crossovers near:

```text
180 Hz, 450 Hz, 1 kHz, 2.4 kHz, 5.5 kHz
```

This produces six adjacent bands. Unity is `32767`. Values over unity provide
boost, up to `65535`.

```c
wmag89_set_eq_gain(&magazine_sound, 2, 36000);
wmag89_set_eq_gain(&magazine_sound, 5, 22000);
```

## Effects

```c
wmag89_set_distortion(&magazine_sound, 340, 9000);
wmag89_set_chorus(&magazine_sound, 3, 1, 2200);
wmag89_set_reverb(&magazine_sound, 22500, 12000, 4000);
```

The chorus is intentionally shallow. A deep chorus turns sliding contact into a
musical `wiu-wiu`; the defaults use it only as microscopic wall/resonance
smearing. Reverb is also kept separate from the physical body tail so an engine
can lower its mix and route the sound into a global room system.

## Memory and binary size from the validation build

On the supplied 64-bit Linux validation compiler:

```text
sizeof(WMag89State) = 9856 bytes
optimized core object text = 8540 bytes
initialized data = 120 bytes
BSS = 0 bytes
```

Sizes can differ slightly by ABI. There are no allocations after or during
initialization.

## Build and test

```sh
make
make test
make previews
```

Windows / MSYS2 MinGW32:

```sh
gcc -O2 -std=c89 -pedantic -Wall -Wextra -Werror \
    -Iinclude src/wmagazine89.c demo/wmagazine89_demo.c \
    -o wmagazine89_demo.exe

./wmagazine89_demo.exe
```

## Preview order

`previews/wmagazine89_all_presets.wav` contains two seconds per preset in this
order:

```text
pistol metal
pistol polymer
SMG steel
rifle aluminum
rifle polymer
sniper box
heavy drum
```

Each section demonstrates insert, seat tap, tug check, and remove.

## Source layout

```text
wmagazine89/
├── include/wmagazine89.h
├── src/wmagazine89.c
├── demo/wmagazine89_demo.c
├── tests/smoke_test.c
├── tests/size_report.c
├── docs/research_basis.md
├── docs/validation.md
├── previews/*.wav
├── Makefile
└── LICENSE
```

## License

CC0 1.0 Universal. See `LICENSE`.
