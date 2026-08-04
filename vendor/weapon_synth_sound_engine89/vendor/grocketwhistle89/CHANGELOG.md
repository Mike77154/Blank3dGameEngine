# Changelog

## v1.2 - airy spatial / dual EQ pass

- Added a second six-band EQ after reverb with crossovers at 120, 300, 900,
  2400 and 6000 Hz.
- Reduced final low-frequency energy in all weapon presets.
- Raised the 2.4-6 kHz edge and above-6 kHz air bands modestly.
- Added weak second and third whistle harmonics.
- Added smoothed fixed-point pitch and amplitude roughness.
- Added a phase-related aerated-tone layer.
- Added a fixed 256-sample plume-air delay buffer with independent left/right
  taps for spatial width.
- Added output-EQ getters and runtime control functions.
- Added `GWH89_PRESET_CHIFLADORA_AIR_REF` as a diagnostic reference preset.
- Replaced the old A/B preview with
  `preview_rpg7_electronic_vs_airy.wav`.
- Expanded API tests for stereo air, output EQ and the seventh preset.

## v1.1 - six-band EQ and optional reverb

- Added the primary six-band contour EQ.
- Added optional three-comb damped reverb.
- Added per-preset EQ and reverb settings.
