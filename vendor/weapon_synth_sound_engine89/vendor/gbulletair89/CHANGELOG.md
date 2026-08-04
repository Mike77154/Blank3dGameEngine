# Changelog

## 1.2

- Added a dedicated two-pole fixed-point low-pass for the aerodynamic body.
- Kept the ballistic crack outside the new body filter so supersonic snaps remain sharp.
- Reduced air and ultra-air EQ gains while raising low-mid body.
- Increased preset attack times by roughly 6-10 ms.
- Added modest makeup level after the darker filtering.
- Retuned all eight presets and regenerated previews.

## 1.1

- Added six-band Q12 fixed-point EQ.
- Increased ADSR attack times for a more aerodynamic swell.
- Increased reverb mix and feedback per preset.
- Added configurable `reverb_tail_ms`.
- Continued chorus/reverb processing after the source ADSR ends.
- Corrected wet-mix scaling from half-Q15 to full-Q15.
- Retuned all eight presets.
- Extended preview files to 0.6 seconds.

## 1.0

- Initial release.
