# Distortion + EQ revision

## Scope lock

Changed:

- the existing `gwf89_drive()` implementation, now `gwf89_distortion()`;
- the existing six-band gains;
- the existing per-preset `drive_q15` values.

Not changed:

- the three noise sources;
- envelopes or micro-hit timing;
- chorus topology or parameters;
- reverb topology or parameters;
- preset count;
- context layout or memory buffers.

No resonator, compressor, limiter, exciter, transient shaper, sample playback,
extra oscillator, or additional effect was introduced.

## Physical/acoustic rationale

Mechanical weapon Foley is a sequence of very short rigid-body contacts. In real
impact sounds, duration, frequency distribution, attack loudness and decay
behaviour all contribute to material and size perception. Because this library
intentionally forbids modal resonators, the closest available controls are the
existing micro-hit timing and the six-band spectral balance.

The new EQ therefore emphasizes:

- 250–800 Hz for receiver, frame and latch mass;
- 800 Hz–5 kHz for the audible `tack/tek` body;
- 5–10 kHz for striker, spring and selector edge;
- less 10–22.05 kHz than before, preventing nonlinear white-noise fizz.

The soft clip reduces crest factor and raises short-term density. Unlike hard
clipping, the rational knee has no flat corner and approaches full scale
smoothly.

## Equal-seed measurements

Measurements compare the old and new compiled C cores using identical preset
seeds and identical 8192-sample renders.

| Preset | RMS change | Crest-factor change |
|---|---:|---:|
| pistol empty | +7.03 dB | -1.55 dB |
| pistol handling | +6.74 dB | -1.08 dB |
| Magnum empty | +8.23 dB | -3.14 dB |
| Magnum latch | +8.62 dB | -2.63 dB |
| sniper empty | +7.69 dB | -2.26 dB |
| sniper bolt dry | +8.72 dB | -3.25 dB |
| SMG empty | +7.50 dB | -2.12 dB |
| SMG selector | +6.93 dB | -1.09 dB |
| launcher empty | +9.19 dB | -3.94 dB |
| launcher latch | +9.54 dB | -2.97 dB |
| shotgun empty | +8.39 dB | -3.15 dB |
| shotgun safety | +7.39 dB | -1.74 dB |

Maximum measured absolute peak across the new individual previews was below
0.87 full scale. The spectral centroid moved downward by roughly 0.24–0.80 kHz
per preset, consistent with the intended reduction of extreme-high hiss and
increase in mechanical body.
