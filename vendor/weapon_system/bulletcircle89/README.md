# bulletcircle89

C89/no-heap geometric spawn helper. Given a center plus right/up basis, radius, slot index and slot count, it returns a point on a circle. It uses integer CORDIC; no float/double or trig runtime is required.

The library is generic: barrels, magic emitters, ring launchers, particle ports and any other circular spawn geometry can use it.
