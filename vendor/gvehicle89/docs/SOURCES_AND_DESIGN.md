# Design notes

The library combines several early/older vehicle-game ideas:

- Halo-style vehicle feel: mass points / fuzzy physics contact against world geometry.
- Gran Turismo-style sim-lite thinking: suspension damping, tire response, driving assists.
- Sega Rally-style material handling: asphalt, gravel, mud, snow, shallow/deep water.
- Daytona-style arcade behavior: forgiving steering and drift-friendly response.
- Wave Race-style water response: wave height samples and buoy probes.

The implementation keeps those ideas cheap with fixed-point math, LUTs, callback probes, and static arrays.
