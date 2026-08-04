# SimSynth particle-body pass 1.4

## Diagnosis from the supplied preset

The preset does not obtain its metallic tick merely by gating white noise.

1. All three oscillators use noise and contribute slightly different layers.
2. The amplifier has an almost instant attack, but decay/sustain/release leave
   a short body instead of falling directly to zero.
3. The filter owns a second envelope. It opens rapidly for the initial edge and
   closes sooner than the amplifier.
4. Band/high content supplies the tick while the closing filter reveals a
   darker body.
5. The result is one coherent particle with an edge and a body, rather than
   several disconnected noise slices.

## Fixed-point translation

Each `gwf89_hit` now owns `gwf89_tone_shape`:

```text
filter attack / decay / hold / release
filter sustain
cutoff floor
cutoff envelope depth
emphasis
high mix
band mix
```

A stable two-stage one-pole network produces approximate low, band, and high
components. The dynamic coefficient is:

```text
alpha = cutoff_base + cutoff_env * tone_envelope
```

The filter envelope begins bright and decays toward the cutoff floor. The
amplitude envelope continues after that point, exposing low/band body.

No trigonometry, lookup table, allocator, floating point, or new oscillator is
required.

## Mechanism tuning

Small detents and springs use higher cutoff and stronger high mix. Frames,
hammers, shotgun receivers, bolt stops, and launcher tubes use lower cutoff,
more band content, longer amplifier hold, and longer release.
