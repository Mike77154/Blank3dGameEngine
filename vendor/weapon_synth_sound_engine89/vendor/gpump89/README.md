# gpump89 v2_real

A compact pump-action mechanism sound synthesizer for low-resource game engines.

This revision removes the sine-bank/modal approach from v1. The core sound is built
from short, aperiodic contact exciters plus stretched rail-friction micrograins.
The included grains were procedurally generated; no firearm recording is embedded.

## Design goal

The audible gesture is intentionally dominated by two masses:

1. rear stop: **CHUK**
2. forward battery/lock: **CLACK**

Unlock, extractor, carrier and lock are quieter secondary contacts. Sliding noise is
kept below the main impacts so the result does not become "noise with tones."

## Core protocol

- ISO C89
- fixed-point/integer DSP only
- no malloc/realloc/free
- no float/double/math.h
- no heap
- caller-owned context
- pull-based signed 16-bit mono output
- deterministic seed
- custom-grain provider hook
- built-in sound bank: approximately 10.3 KiB of signed 8-bit exciters

The demo and tests use the standard library only outside the audio core.

## Build

```sh
make
make check
./gpump89_demo 0 tight_dry.wav
```

Preset IDs:

- `0` tight dry
- `1` loose service
- `2` heavy
- `3` with shell ejection
- `4` cinematic dry

## Basic integration

```c
gpump89_context pump;
gpump89_params p;
gpump89_i16 mix_buffer[512];

gpump89_preset(&p, GPUMP89_PRESET_TIGHT_DRY, audio_rate);
gpump89_init(&pump, audio_rate, p.seed);
gpump89_start_cycle(&pump, &p);

/* In the audio callback: */
gpump89_render_i16(&pump, mix_buffer, 512);
```

## Provider mode

Call `gpump89_set_provider()` with a callback that supplies a static signed 8-bit
grain for any stage. Returning zero falls back to the built-in procedurally generated
grain. The provider owns the memory and must keep it valid during playback.

This makes the same scheduler usable with:

- your own Foley recordings;
- engine asset banks;
- platform ROM data;
- different weapon families;
- animation-synchronized custom contacts.

See `docs/PROVIDER_ABI.md`.
