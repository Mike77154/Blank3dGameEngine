# Cursor overflow fix — v1.4.3

## Cause

The realtime showcase stored source position as a single unsigned 32-bit Q16 value.
At 44.1 kHz it overflowed after 65,536 source frames (1.486077 seconds).
The five-second rocket-blast bank therefore restarted from frame zero several times,
even though the event scheduler dispatched only one impact.

## Correction

The source provider now stores:

- a 32-bit integer frame cursor;
- a 16-bit Q16 fractional accumulator;
- the existing 32-bit Q16 pitch step.

Carry from the fractional accumulator advances the integer cursor without shifting the
full frame index into a 32-bit Q16 container. Long banks stop at their true final frame.

## Regression evidence

- Old correlation with the block 65,536 frames later: `0.999913`.
- Corrected correlation at the same offset: `0.002949`.
- Scheduled rocket impacts in the focused demo: `1`.
- Clipped samples: `0`.
- PCM peak: `28151`.

The DSP in `wsound_rocketblast89` was not responsible and was not weakened. Rumble,
crackle, SVF, LFO and room tail remain enabled.
