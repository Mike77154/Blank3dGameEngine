# Design notes

## Why three noise sources?

White noise itself is unpitched. The “grave / normal / agudo” model is therefore implemented as three independent white-noise streams with different spectral treatment:

- low: one-pole smoothing for receiver/body mass;
- normal: unchanged noise for the broad mechanical transient;
- high: first difference for sharp striker/spring edges.

## Why a six-band filter bank instead of six runtime biquads?

The core must avoid floating-point coefficient generation and remain very small. Five fixed one-pole low-pass states create six complementary difference bands. Equal gains telescope back toward the original signal, while preset gains reshape the transient without resonant ringing.

## Why micro-hit patterns?

A realistic `tick`, `tack`, `tek` or `teke` is usually not one homogeneous burst. The presets stack two to four tiny envelopes with slightly different low/white/high balances and offsets. This creates trigger movement, striker impact, spring return and latch contact without importing samples.

## Effects policy

Chorus and reverb are intentionally subtle. The chorus is a low-wet modulated delay; the reverb is two short comb paths followed by one all-pass diffuser. They add metal size and a little acoustic separation without turning the dry mechanism into a sci-fi sweep or a room wash.


## Distortion revision 1.1

The old quadratic drive mostly behaved as a gentle level-dependent compression stage. It was replaced by one symmetric memoryless rational soft clip:

1. fixed-point pre-gain;
2. an amount-dependent knee;
3. a smooth saturating curve approaching signed 16-bit full scale;
4. dry/wet blending derived from the same amount parameter.

All arithmetic uses signed 32-bit intermediates with bounded multiplication. The pre-clipped magnitude is explicitly limited before the rational product, so no C89 `long long` is required.

The distortion is intentionally symmetric to avoid adding a DC component. It is applied at the same point in the signal chain previously occupied by `gwf89_drive`; no new processing block was inserted.

The six-band EQ presets were shifted toward low-mid and upper-mid energy while the extreme-high band was reduced. This preserves the heavy receiver/latch body and the striker edge while avoiding excessive white-noise fizz after nonlinear processing.
