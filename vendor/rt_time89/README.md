# rt_time v2.1 protocol89

Sanitized variant of the supplied `rt_time v2`.

## Why it was sanitized

The supplied v2 was logically strong but did **not** satisfy Blank3D protocol89:

- `double` throughout public API and implementation.
- `int64_t` / `uint64_t`.
- `stdint.h`.
- fixed-step based on floating-point seconds.

The protocol89 variant preserves the architecture and semantics while using:

- C89.
- 32-bit integer monotonic tick provider.
- Q16.16 scale / seconds views.
- millisecond simulation accumulation.
- no heap.
- no floating point.
- no explicit 64-bit arithmetic.

It retains raw-vs-simulated time, pause, scale, clamp, named helper timers and
fixed-step accumulation. A wrapping 32-bit source is supported.

## Delta time

`rt_time` already owns universal delta-time semantics:

- `rt_time_raw_delta_ms()`
- `rt_time_delta_ms()`
- `rt_time_raw_delta_q16()`
- `rt_time_delta_q16()`
- configurable clamp
- time scale
- pause
- `rt_fixed_step`

For Blank3D this makes a second timekeeping authority called
`UniversalDeltaTime89` unnecessary. The engine bridge promotes this sanitized
`rt_time` as the universal delta-time source instead.

## Build

```sh
gcc -std=c89 -pedantic-errors -Wall -Wextra -Werror \
    -I. rt_time.c tests/test_rt_time.c -o test_rt_time
```
