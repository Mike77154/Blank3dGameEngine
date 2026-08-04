# Changelog

## 1.6.0

- Calibrated contact timing by real mechanism order.
- Added per-contact rigid-body wet, feedback and damping profiles.
- Added three deterministic variants per preset.
- Added slow, normal and fast timing through `gwf89_trigger_ex()`.
- Added bounded seeded variation in timing, level, cutoff, release and body.
- Added wet-only room stem and caller-controlled final room send.
- Kept exactly three white-noise excitation streams and fixed-point C89.

## 1.5.0

- Brighter per-contact and master six-band EQ.
- Amplitude releases lengthened about 100% versus v1.4.
- Tone releases lengthened about 65%.

## 1.4.0

- Added independent amplitude and tone/filter envelopes per micro-contact.
- Added a two-pole low/band/high particle filter with cutoff envelope.

## 1.3.0

- Added four static inharmonic damped-delay metal-body modes.
- Added small-space chorus/reverb processing.

## 1.2.0

- Split mechanisms into ordered physical micro-contacts.
- Added per-contact six-band EQ and distortion.

## 1.1.0

- Added fixed-point soft clipping and revised six-band EQ.

## 1.0.0

- Initial three-noise mechanical Foley synthesizer.
