# TimeVerbs89

Agnostic time vocabulary for DSL bridges.

The core does not own a clock, timers, filesystem, renderer or DSL. It only
normalizes semantic names such as:

- `time_pause`, `time_resume`, `time_scale`
- `timer_start`, `timer_loop`, `cooldown_set`
- `timer_done`, `timer_fired`, `cooldown_ready`
- `every_ticks`, `every_ms`, `every_frames`, `every_seconds`
- `clock_hour_eq`, `clock_minute_eq`, `clock_second_eq`

Blank3D binds these names to its time provider and shared GameVerbs89 bus,
letting RPYL/FPIL/DDSL2 share vocabulary where their argument models allow it.
