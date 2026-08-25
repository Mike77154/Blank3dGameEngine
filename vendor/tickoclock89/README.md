# TickOClock89

Agnostic multi-view simulation clock. One update exposes the same simulated
time simultaneously as:

- configurable ticks
- milliseconds
- simulation frames / frametime
- seconds/minutes/hours
- wall-style HMS + day rollover

It owns no OS clock. Feed it the delta produced by `rt_time`.

C89, no heap, no float/double, no 64-bit types.
