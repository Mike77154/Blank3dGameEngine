# Blank3D Time Stack89 integration

## Purpose

Blank3D no longer owns wall-clock sampling, delta-time policy, timer bookkeeping,
or DSL-facing time vocabulary in the main loop.  The time stack is split into
small provider-driven C89 libraries:

```text
platform clock provider
        |
        v
    rt_time89
 raw time / simulation delta / pause / scale / clamp / fixed step
        |
        v
   TickOClock89
 ticks / milliseconds / simulation frames / H:M:S.ms / periodic pulses
        |
        +-------------------+
        |                   |
        v                   v
 TimeClocker89          TimeVerbs89
 timers/cooldowns/      vocabulary only
 stopwatches/alarms          |
        |                   v
        +------------> GameVerbs89
                             |
                    RPYL / FPIL / DDSL2
```

`src/blank3d_time89.*` is the Blank3D orchestration bridge.  The platform only
supplies a wrapping 32-bit monotonic tick callback (`rt_time_source`).

## Protocol audit of the supplied libraries

### timeclocker -> vendor/timeclocker89

The supplied timeclocker was already C89/no-heap/no-float, but its integer
aliases used `long`/`unsigned long` as if those types were guaranteed 32-bit.
That is true on Win32 but false on LP64 systems.  The vendored protocol89 copy
uses `int`/`unsigned int`, verifies that they are at least 32-bit with
`<limits.h>`, and retains the original fixed-capacity API, tests, examples and
documentation.

### rt_time -> vendor/rt_time89

The supplied rt_time already contained the semantics needed for a universal
DeltaTime authority: raw delta, simulation delta, pause, time scale, maximum
step clamp, timers and a fixed-step accumulator.  However, its implementation
used `double`, `stdint.h`, `int64_t` and `uint64_t`.

The protocol89 port replaces those with integer/Q16.16 math and a wrapping
32-bit tick provider.  No second `UniversalDeltaTime89` library was added,
because that would duplicate authority.  `rt_time89` is the universal delta
source for the engine.

## Delta-time policy

At startup Blank3D attaches its platform clock to `rt_time89`.  On Win32 the
provider currently calls `GetTickCount()`.  No Win32 symbol leaks into
`rt_time89` itself.

The main loop now performs:

```c
blank3d_time89_update(&g.time_system);
blank3d_timeverbs89_pump_alarms(&g.time_verbs);
g.frame_ms = blank3d_time89_frame_ms(&g.time_system);
g.dt = blank3d_time89_delta_q16(&g.time_system) >> 4;
```

`g.dt` remains the existing Q-format expected by Blank3D movement/physics code,
so current consumers do not need private clocks.

The integration intentionally preserves the historical Blank3D stability
policy:

- normal running has a minimum effective simulation step of 1 ms;
- pause or zero time-scale yields an actual zero step;
- the current host initializes the maximum simulation step to 50 ms.

The raw wall-clock delta remains available separately through
`blank3d_time89_raw_delta_ms()`.

## Fixed-step support

`rt_time89` also exposes `rt_fixed_step`.  It maintains a fixed millisecond
simulation quantum, maximum catch-up steps, interpolation alpha, total step
count and dropped-update count.  Blank3D keeps its existing variable-step
consumers for compatibility in this integration, but physics or future
subsystems can migrate to the fixed-step API without introducing a second
clock.

## TickOClock89

`TickOClock89` converts simulation delta into multiple authoring views without
owning a platform clock:

- global ticks at a configurable tick rate;
- global milliseconds;
- simulation frame count;
- per-update tick/frame deltas;
- H:M:S.millisecond clock representation;
- periodic predicates (`every_ticks`, `every_ms`, `every_frames`,
  `every_seconds`, `every_minutes`).

The default Blank3D integration initializes it at 60 ticks/second.  The tick
rate can be changed at runtime by TimeVerbs89.

## TimeClocker89

TimeClocker89 consumes tick/frame increments from TickOClock89 and provides
fixed-capacity named state:

- one-shot timers;
- loop timers;
- cooldowns;
- stopwatches;
- frame/tick delays;
- alarms carrying an action name and three integer payload values.

Pending alarms are consumed by `blank3d_timeverbs89_pump_alarms()` and sent
through the shared GameVerbs89 action bus.  The alarm therefore triggers the
same semantic action vocabulary used by the DSL bridges rather than calling a
Blank3D subsystem directly.

## TimeVerbs89 vocabulary

The vendor is vocabulary-only and engine agnostic.  Blank3D implements the
verbs through its time bridge.

### Actions

```text
time_pause
time_resume
time_toggle
time_scale
time_reset
time_tick_rate
time_set_hms

timer_start
timer_once
timer_loop
cooldown_set
stopwatch_start
frame_delay
tick_delay
alarm_set

timer_stop
timer_pause
timer_resume
timer_reset
timer_clear
```

Accepted duration units include `ticks`, `ms`, `frames`/`frametime`, `seconds`,
`minutes` and `hours` where meaningful.

### Conditions

```text
time_paused
timer_exists
timer_active
timer_done
timer_fired
timer_ready
cooldown_ready
alarm_pending

every_tick
every_ticks
every_frame
every_frames
every_ms
every_second
every_seconds
every_minutes

global_ticks_gte
global_frames_gte
global_ms_gte
global_seconds_gte
global_minutes_gte

clock_hour_eq
clock_minute_eq
clock_second_eq
```

## DSL boundary

TimeVerbs89 is registered into the same `GameVerbs89` registry consumed by the
RPYL, FPIL and DDSL2 host bridges.  RPYL/FPIL can forward textual arguments.
DDSL2 actions can also forward their action payload.

The current DDSL2 condition-import path imports condition names but does not
transport condition arguments.  Therefore zero-argument conditions such as
`time_paused`, `every_tick`, `every_frame` and `every_second` are directly
usable there, while argument-bearing conditions such as `timer_done door` or
`every_ms 500` require a later DDSL2 condition-argument transport extension.
This is a pre-existing bridge limitation, not a TimeVerbs89 parser limitation.

## Protocol89 properties

The four time vendors are compiled independently with:

```text
-std=c89 -pedantic-errors -Wall -Wextra -Werror
```

Their core source/header audit rejects:

- malloc/calloc/realloc/free calls;
- float/double;
- long long;
- int64_t/uint64_t;
- stdint.h.

No filesystem, renderer or operating-system dependency is present in the four
vendor cores.

The Blank3D GameVerbs89 host boundary still inherits the engine's existing
legacy `long value_q16` ABI.  On the canonical MinGW32 target `long` is 32-bit;
on LP64 hosts it is wider.  The time vendors themselves do not use that type.
An engine-wide GameVerbs89 ABI cleanup can remove that pre-existing portability
debt separately without changing the TimeVerbs89 vocabulary.

## Regression note

The repository's existing `make test-core` exits with test program code 42 in
both the immediately previous skybox-recipe build and this time-stack build.
The failure predates this integration.  Time-specific gates and the selected
movement/weapon/physics/DSL regressions pass; see `TIME_STACK89_QA.txt`.
