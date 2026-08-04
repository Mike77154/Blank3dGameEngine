# Rocket Whistle v1.2 integration — engine v1.8.3

## Replacement

`vendor/grocketwhistle89/` was deleted and recreated from
`grocketwhistle89_v1_2_airspace_dualeq_c89_fixedpoint.zip`. No v1.1 runtime
source remains in the vendor tree.

## New signal features

- weak second and third harmonics;
- smoothed pitch and amplitude roughness;
- phase-related aerated tone;
- independent 256-sample plume-air stereo delay;
- primary six-band source EQ;
- optional three-comb reverb;
- secondary six-band finishing EQ after reverb;
- seven presets, including the diagnostic chifladora reference.

## Event facade

`WSSE89_EVENT_ROCKET_WHISTLE_FX` addresses an active whistle by `instance_key`.
Each of its three sections uses mode 0 to preserve the preset, mode 1 to disable
that stage, and mode 2 to apply custom values:

- primary EQ;
- output EQ;
- reverb.

The low-level v1.2 API remains directly available through
`weapon_synth_sound_engine89.h`.

## Sequence tuning

The RPG timeline uses the airy RPG-7 preset. Final EQ rejects sub-rumble,
retains 300–900 Hz body and emphasizes 2.4–6 kHz air moderately. The bank is
normalized below the old trajectory level so ignition and impact remain the
principal events.
