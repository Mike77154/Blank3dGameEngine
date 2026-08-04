# gpaah89 BASE PRESETS v5 design notes

## Topology

```text
noise low  --\
noise mid  ----> transient-only correlation -> independent envelopes -> sum
noise high --/

sum
  -> six parallel non-resonant band passes
  -> envelope-driven parallel soft clipping
  -> feed-forward stereo chorus
  -> six delayed FIR reflections with zero feedback
  -> stereo PCM
```

## Deeper shotgun

The shotgun keeps a hard common transient, but shifts its six broad bands to
25-85, 85-220, 220-600, 600-1600, 1600-4200 and 4200-9500 Hz. The low and
low-mid layers receive longer decay/release times; the high layer remains short.
This adds weight without a sine sub oscillator or resonant filter.

## Deeper rocket launcher

The rocket profile uses bands at 20-70, 70-180, 180-500, 500-1400,
1400-4000 and 4000-9000 Hz. Its low-noise envelope lasts longest and its six
FIR reflections extend to 180 ms. Upper-band gain is reduced so the expansion
reads as pressure rather than white-noise rasp.

## Non-resonant EQ

Each band remains the difference between two first-order low-pass states:

```text
band(x) = LP(high edge, x) - LP(low edge, x)
```

There is no Q control, resonant peak, biquad or feedback.

## Effects

Distortion is one parallel wet/dry soft clipper. Chorus uses two feed-forward
ring-buffer reads. Reverb is six fixed delayed copies with descending Q15 gains
and no feedback. No effect can self-oscillate.

## Determinism

The same preset, seed, sample rate and trigger schedule produce identical PCM.
