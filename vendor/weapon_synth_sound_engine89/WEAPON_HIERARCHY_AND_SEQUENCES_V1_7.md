# Weapon hierarchy and complete sequences — v1.7.0

## Goal

Keep every mechanical cue readable at close range without allowing a slide,
bolt, pump, carrier, feed system or casing to become louder than the muzzle
report or explosive event it belongs to.

## Physical mix order

The default Hybrid profile applies this class hierarchy before per-event gain:

| Class | Gain Q15 | Purpose |
|---|---:|---|
| Report | 32767 | Muzzle blast / launch ignition |
| Explosion | 32767 | Grenade, rocket and 40 mm impact |
| Impact | 28200 | Projectile/material response |
| Ricochet | 25000 | Fast secondary transient |
| Mechanism | 20800 | Pump, slide, bolt, feed and carrier |
| Foley | 18800 | Selector, latch and handling |
| Casing | 17200 | Shell and brass contacts |
| Ambience | 16800 | Background and persistent world bed |

Realistic lowers close-detail sweetening; Cinematic raises body detail while
preserving the same physical order. `WSSE89_EVENT_WEAPON_MODE` now updates DNA,
acoustic profile and this class mixer together.

## Complete sequence timing

- Pistol: handling/rack, report, short-cycle slide, ejection and casing landing.
- Magnum: cylinder/latch, report, deliberate action and manual brass handling.
- Pump shotgun: shell insert, chambering pump, report, extraction/ejection pump.
- Machine gun: selector/feed, ten-round burst, per-cycle feed action and casings.
- Sniper rifle: chambering bolt, report, delayed manual bolt cycle and casing.
- Hand grenade: pin/lever details, fuse interval, M67-style open blast.
- Rocket launcher: latch/ready, launch ignition, flight interval and rocket impact.
- Grenade launcher: open/latch, low-pressure launch, flight, 40 mm blast and extraction.

These timings are representative gameplay timings informed by mechanism order,
not universal measurements. The host may schedule each event independently.

## Reference basis

The design was checked against:

- NIOSH firearm impulse-noise investigations and peak-level measurements.
- Maher and related gunshot-acoustics work separating muzzle blast, ballistic
  wave and quieter mechanical action.
- RemArms Model 870 official owner manual for pump/bolt/carrier ordering.
- Winchester Model 70/XPR official manuals for manual bolt sequencing.
- U.S. Army M240 documentation for gas-driven feed/extract/eject cycles.
- U.S. Army M67 grenade and M72 LAW documentation for delayed fuze and
  launch/impact event separation.

No external recordings are copied or redistributed. All WAV files are rendered
procedurally from the included C89 libraries.

## Generated previews

`audio/07_complete_weapon_sequences_balanced.wav` contains every family in order.
`audio/08` through `audio/15` contain isolated complete sequences.
`audio/16_shotgun_v1_6_2_vs_v1_7_balance_ab.wav` compares the previous shotgun
showcase against the new hierarchy.
