# Metallic body pass 1.3

## Problem

The 1.2 physical pass gave every mechanism an ordered set of contacts, but a
very short independently generated noise burst can still be perceived as gated
or interrupted broadband noise. Real metal retains correlated vibration after
the exciting contact has ended.

## Adapted hi-hat principle

Classic synthetic cymbal/hi-hat designs create a dense inharmonic source, split
or emphasize spectral regions, and use a short amplitude contour. Physical and
modal approaches likewise represent rigid bodies with multiple decaying modes.

This implementation adapts that principle without adding sampled material or a
pitched oscillator bank:

1. the existing three white-noise-derived streams create each contact;
2. the contact excites four short recirculating delay paths;
3. prime/inharmonic delay choices avoid one obvious musical pitch;
4. a one-pole damping state inside each feedback loop shortens high-frequency
   persistence;
5. alternating output polarities reduce simple comb reinforcement;
6. the body is mixed underneath the original dry transient.

## Space

Version 1.2 used delays of only a few milliseconds in the reverb stage, which
could behave more like metallic coloration than audible room. Version 1.3 makes
all three delay lengths preset-controlled and uses a restrained small-space
range. Wet and feedback amounts remain low.

## Fixed-point implementation

- four 64-sample static delay lines;
- Q15 wet, feedback, damping, and mode weights;
- no trigonometry or lookup table;
- no allocator;
- white noise remains the sole excitation source.

## Preset intent

Small controls use shorter/brighter mode delays. Larger receivers, shotguns,
bolts, and launcher tubes use longer mode delays and slightly stronger feedback.
The launcher has the largest body and room, while safety and selector clicks
remain compact.

## References consulted

- Gordon Reid, "Practical Cymbal Synthesis", Sound On Sound.
- S. Bilbao, "A Modular Percussion Synthesis Environment", DAFx-09.
- Diaz et al., "Rigid-Body Sound Synthesis with Differentiable Modal
  Resonators", 2022.
