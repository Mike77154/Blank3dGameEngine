# Phase 3: Weapon DNA, correlated variation and validation

`weapon_synth_sound_engine89 v1.6.0` adds a physical identity layer and a
heapless analysis layer on top of the Phase 1/2 acoustic world.

## Research basis

The implementation deliberately does not embed or redistribute field
recordings. It uses published research as a guide for what must vary and what
must be measured:

- C3GD describes more than 8,000 field-collected observations from 28 firearms
  and 16 calibers, with firearm, cartridge, microphone and location metadata:
  https://arxiv.org/abs/2606.18135
- Gun-type acoustic research treats muzzle blast and shockwave properties as
  varying with firearm, ammunition and shooting direction:
  https://arxiv.org/abs/2506.20609
- Steam Audio separates distance, air absorption, directivity, occlusion and
  transmission; Phase 1/2 already follows that separation:
  https://valvesoftware.github.io/steam-audio/doc/capi/simulation.html
- EBU R128 motivates keeping level, dynamic range and true-peak-style safety as
  separate descriptors rather than using one peak meter as a quality score:
  https://tech.ebu.ch/publications/r128

The numeric target envelopes shipped here are editable starter targets, not
claims of certified averages from those datasets. A project can replace them
with statistics measured from its own licensed reference corpus.

## Physical Weapon DNA

`wsounddna89_profile` exposes:

- profile/class and mechanical action;
- bore in hundredths of a millimetre;
- barrel length;
- projectile speed;
- cyclic rate;
- propellant energy and muzzle pressure;
- mechanical mass;
- suppressor and muzzle-brake amounts;
- supersonic probability/strength.

Ten defaults are included: compact pistol, service pistol, magnum, SMG,
carbine, rifle, sniper, shotgun, heavy and launcher.

A profiled shot produces one correlated family:

```text
energy latent
  +-- pressure
  +-- gas
  +-- receiver/mechanism
  +-- thump
  +-- ballistic crack
  +-- brightness
  +-- tail energy
  +-- pitch/cycle microvariation
```

The four exposed variation values are correlated but not identical:
`energy_variation_q15`, `powder_variation_q15`,
`mechanism_variation_q15`, and `environment_variation_q15`.

Same profile + same mode + same seed produces the same sequence, making the
system suitable for replays and deterministic network simulation.

## Whole-report application

`gssr89_apply_dna()` maps one DNA shot into the complete report stack. It
changes layer gains and also modifies copied preset data before initialization:

- report envelope timing and upper EQ;
- weapon-body modal delays and exciter/tail time;
- muzzle-gas decay and cutoffs;
- ballistic N-wave width and air cutoff;
- late-tail duration;
- thump duration and frequency sweep.

The original preset tables remain immutable.

## Runtime events

Five events were added:

```text
WSSE89_EVENT_WEAPON_PROFILE
WSSE89_EVENT_WEAPON_MODE
WSSE89_EVENT_WEAPON_FIRE
WSSE89_EVENT_METRICS_ENABLE
WSSE89_EVENT_METRICS_RESET
```

Example:

```c
wsounddna89_profile profile;
wsse89_event event;
gv89_handle handle;

wsounddna89_profile_defaults(WSOUNDDNA89_PROFILE_RIFLE, &profile);

event.type = WSSE89_EVENT_WEAPON_PROFILE;
event.seed = 0U;
event.data.weapon_profile = profile;
wsse89_dispatch(&audio, &event, 0);

event.type = WSSE89_EVENT_WEAPON_MODE;
event.data.weapon_mode = WSOUNDDNA89_MODE_HYBRID;
wsse89_dispatch(&audio, &event, 0);

event.type = WSSE89_EVENT_WEAPON_FIRE;
wsse89_weapon_fire_defaults(&event.data.weapon_fire);
event.data.weapon_fire.pan_q15 = -8000;
wsse89_dispatch(&audio, &event, &handle);
```

`WEAPON_FIRE` automatically triggers the coherent report, pressure pulse,
receiver excitation and matching bare/brake/suppressor muzzle-device layer.

## Fixed-point metrics

`wsoundmetrics89` is caller-owned and allocation-free. It measures:

- sample peak and clipped sample count;
- decimated integer RMS estimate;
- crest factor Q8;
- low/mid/high energy ratios;
- brightness proxy;
- onset-to-peak attack time;
- peak-to-last-significant-energy decay time;
- zero-crossing ratio.

It is intended for regression tests and calibration, not as a standards-
compliant LUFS meter. `wsoundmetrics89_target_defaults()` creates editable
starter envelopes from Weapon DNA and mode. `validate()` returns failure bits;
`score_q15()` gives a graded comparison.

## Modes

- `REALISTIC`: smallest variation, reduced sweetening and shorter tails.
- `HYBRID`: physical identity with moderate perceptual reinforcement.
- `CINEMATIC`: stronger thump, pressure and tail while retaining the same
  physical family and deterministic variation.

Setting `WSSE89_EVENT_WEAPON_MODE` also selects the matching Phase 1/2 acoustic
profile so the source and world do not disagree.

## Preview

`audio/16_phase3_realistic_hybrid_cinematic.wav` contains the same rifle DNA
and seed in Realistic, Hybrid and Cinematic order. The generated metrics are in
`PHASE3_METRICS_V1_6.json`.
