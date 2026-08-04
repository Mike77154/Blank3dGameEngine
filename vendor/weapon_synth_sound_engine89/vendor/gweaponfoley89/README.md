# gweaponfoley89 — real-reference calibration 1.6

A compact **dry mechanical weapon-Foley synthesizer** made from exactly three
seeded white-noise streams and fixed-point processing.

Version 1.6 keeps the bright SimSynth-style particle established in 1.5 and
calibrates the event structure toward real firearm mechanisms: striker release,
hammer fall, revolver indexing, bolt lift/rail/stop/lock, selector detents,
shotgun controls and tubular launcher latches.

It does **not** synthesize gunshots, explosions, cartridge ejection, magazine
insertion, shell drops, ammunition handling, or the shotgun pump/chamber sound.

## Hard constraints

- C89 core
- fixed-point only
- exactly three seeded white-noise sources
- no `malloc`, `calloc`, `realloc`, or `free`
- no heap ownership
- no `float`, `double`, `math.h`, `stdio.h`, or `stdlib.h` in the core
- caller-owned context and output buffers
- fixed 44,100 Hz mono signed 16-bit PCM

## Signal path

```text
three seeded white-noise streams
  |- smoothed low-body noise
  |- direct contact noise
  `- differentiated high-edge noise
          |
up to eight ordered physical micro-contacts
          |
  independent amplitude + tone envelopes
          |
  two-pole low / band / high particle filter
          |
  per-contact six-band EQ + soft distortion
          |
  per-contact four-mode inharmonic metal body
          |
master six-band EQ + light distortion + chorus
          |                         |
        dry stem               room stem
          |                         |
          `------ room send --------'
                    |
              mixed PCM output
```

## New in 1.6

- Per-contact rigid-body wet level, feedback and damping. A spring no longer
  rings like a hammer, and a tube no longer decays like a selector detent.
- Three deterministic variants per preset:
  - `0`: tight / slightly brighter
  - `1`: neutral reference
  - `2`: heavier / slightly longer
- `slow`, `normal`, and `fast` mechanism timing.
- Tiny seeded differences in contact timing, gain, cutoff, release and body.
- Separate dry and room stems through `gwf89_process_sample_stems()`.
- Configurable final room send through `gwf89_set_room_send()`.
- Legacy `gwf89_trigger()` and `gwf89_trigger_seeded()` remain available.

This is an ABI revision because `gwf89_hit` and `gwf89_context` gained fields.

## Included presets

- `pistol_empty`
- `pistol_handling`
- `magnum_empty`
- `magnum_latch`
- `sniper_empty`
- `sniper_bolt_dry`
- `smg_empty`
- `smg_selector`
- `launcher_empty`
- `launcher_latch`
- `shotgun_empty`
- `shotgun_safety`

## Minimal use

```c
#include "gweaponfoley89.h"

static gwf89_context foley;
static gwf89_s16 block[256];

void boot(void)
{
    gwf89_init(&foley, 0x12345678UL);
}

void bolt_fast(void)
{
    gwf89_trigger_ex(&foley, GWF89_SNIPER_BOLT_DRY,
                     0x12345678UL, 1U, GWF89_SPEED_FAST);
}

void audio_callback(void)
{
    gwf89_process(&foley, block, 256UL);
}
```

## Dry/room stems

```c
gwf89_s16 dry;
gwf89_s16 room;
gwf89_s16 mixed;

mixed = gwf89_process_sample_stems(&foley, &dry, &room);
```

`room` is the wet-only small-space return. The default mixed output is compatible
with the previous additive room path. Set the final room send to zero for a dry
mechanical signal:

```c
gwf89_set_room_send(&foley, 0);
```

## Listening files

- `previews_real_reference/00_catalog_all_presets_real_reference.wav`
- `comparisons_real_reference/00_AB_catalog_v1_5_then_v1_6.wav`
- `variants_real_reference/` for tight, neutral and heavy bolt variants
- `speeds_real_reference/` for slow/normal/fast examples
- `stems_real_reference/` for dry, room and mixed bolt renders

A/B renders use the same seeds and are not normalized. Version 1.5 plays first,
then version 1.6.

## Build

```sh
make
make render_previews
```

Validated with strict C89 warnings as errors. See
`docs/VALIDATION_1_6.txt`.

## License

CC0-1.0.
