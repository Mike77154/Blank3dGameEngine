# GGunTuberotator89

A compact dry mechanical rotating-barrel / tube-rotor synthesizer for game audio.
It models **three connected events**:

1. rotational start / acceleration;
2. stable central loop;
3. rotational fall / deceleration.

The sound is intentionally closer to barrels, vanes, or helicopter-like passes
cutting air than to a clean electric motor. Each barrel-passage pulse retriggers:

- a zero-attack, short-decay full-band noise burst;
- a second darker noise burst updated at half rate and low-pass filtered;
- a gated C2/C3-range sine thump;
- six-band subtractive EQ;
- restrained fixed-point saturation;
- a short, dark two-tap reverb.

## Constraints

- ISO C89 source style
- integer fixed-point DSP only
- no `float` or `double`
- no `malloc`, `calloc`, `realloc`, or `free`
- no hidden heap ownership
- caller-supplied `ggtr89_context`
- fixed-size delay buffers embedded in the context
- mono signed PCM16 output
- sample rates from 8 kHz to 48 kHz

## Basic use

```c
ggtr89_context rotor;
ggtr89_config cfg;
ggtr89_i16 pcm[512];

ggtr89_config_preset(&cfg, 44100, GGTR89_PRESET_MEDIUM);
ggtr89_init(&rotor, &cfg);
ggtr89_start(&rotor);       /* accelerates, then enters loop automatically */
ggtr89_render(&rotor, pcm, 512);
ggtr89_stop(&rotor);        /* decelerating fall */
```

## Presets

- `GGTR89_PRESET_LIGHT`: faster, smaller three-tube rotor; C2/G2-ish body.
- `GGTR89_PRESET_MEDIUM`: six-tube dry Gatling-like mechanism.
- `GGTR89_PRESET_HEAVY`: slower seven-tube mass with darker low-mid impacts.

All parameters remain editable through `ggtr89_config` before `ggtr89_init()`.
The six EQ gains use Q12 (`4096 == 1.0`).

## Build

```sh
make
make test
make preview

# MSYS2 MinGW32 / Windows cmd
build_mingw32.bat
```

Strict check used for this package:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
  -Iinclude src/gguntuberotator89.c tests/ggtr89_smoke.c -o ggtr89_smoke
```

## Physical/acoustic design basis

- Modern externally powered Gatling systems use a rotating multi-barrel cluster.
- Barrel/bolt motion is mechanically tied to rotation through cam tracks.
- A six-barrel system distributes the cycle across six barrel passages per
  revolution, producing a low periodic pulse train before muzzle blast is added.
- Rotors and fans reinforce tones at passage frequency and harmonics, while
  turbulent interaction contributes broadband noise.

Reference pages consulted during design:

- General Dynamics Ordnance and Tactical Systems, M61A1/M61A2:
  https://www.gd-ots.com/armaments/aircraft-guns-gun-systems/m61a1/
- Dillon Aero, M134D:
  https://dillonaero.com/m134d-standard-7-62-x-51mm/
- National Museum of the U.S. Air Force, M61A1 Vulcan:
  https://www.nationalmuseum.af.mil/Visit/Museum-Exhibits/Fact-Sheets/Display/Article/579640/m61a1-vulcan-cannon/
- NASA NTRS, fan/rotor passage-frequency acoustics:
  https://ntrs.nasa.gov/citations/20110016852
- U.S. Patent 3,611,871, Gatling cam-track mechanism:
  https://patents.google.com/patent/US3611871A/en

## Memory

`sizeof(ggtr89_context)` is implementation-dependent, but the two reverb buffers
account for 8192 bytes when `short` is 16-bit. The validated build measured 8424
bytes for the complete caller-owned context. No memory is allocated internally.

## License

CC0 1.0 Universal. See `LICENSE`.
