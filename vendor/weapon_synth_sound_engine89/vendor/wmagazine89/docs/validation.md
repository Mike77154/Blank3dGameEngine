# Validation report

Validation date: 2026-07-19

## Strict compilation

Compiler:

```text
gcc (Debian 14.2.0-19) 14.2.0
```

Flags:

```text
-O2 -std=c89 -pedantic -Wall -Wextra -Werror
```

Targets compiled successfully:

```text
src/wmagazine89.c + demo/wmagazine89_demo.c
src/wmagazine89.c + tests/smoke_test.c
```

The smoke test exercised every combination of:

```text
7 presets x 5 built-in actions
```

It also exercised a caller-supplied custom event sequence.

## Constraint audit

Core source characteristics:

```text
no malloc
no realloc
no free
no heap API
no float
no double
no math.h
no stdlib.h
no stdio.h
no string.h
no variable-length arrays
no C99 line comments
no global mutable DSP state
```

The constant sine table, preset tables, and event patterns are read-only.
Mutable delay lines and filters live inside `WMag89State`, supplied by the
caller.

## Memory

Measured on the validation ABI:

```text
WMag89State = 9856 bytes
WMag89Event = 20 bytes
```

Optimized object:

```text
text = 8540 bytes
data = 120 bytes
bss  = 0 bytes
```

## Preview signal checks

All WAV files are:

```text
RIFF/WAVE PCM
mono
16 bit
44100 Hz
```

Peak scan found no samples at positive or negative full scale. The combined
preview peak remained below 12000 PCM16, leaving deliberate integration
headroom for weapon, cloth, bolt, and room layers.

## Implementation notes

- The core uses a 256-entry signed sine table and 16-bit phase accumulators.
- One-pole coefficients are calculated with integer arithmetic at init.
- The EQ is a parallel six-band decomposition, not six biquad peaking filters.
- The modal bank is intentionally four modes to keep cost predictable.
- Chorus uses a fixed 512-sample line and triangle modulation.
- Reverb uses three fixed comb lines and one fixed all-pass line.
- Retiggering replaces the active source sequence while existing reverb memory
  continues naturally.
