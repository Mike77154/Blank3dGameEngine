# Research and design mapping

- A firearm event is split into muzzle report and, for supersonic projectiles, a separate ballistic shock wave. `wsoundprojectile89` therefore exists independently from the weapon report.
- `wsoundprop89` models arrival delay, distance loss, direction and obstruction.
- `wsoundroom89` uses a low-order image-source-inspired tap pattern plus a feedback tail, not convolution.
- `wsoundimpact89` and `wsoundreceiver89` use compact modal/delay resonators: an impulse excites material/structure-dependent decaying modes.
- `wsoundcombatbus89` protects new transients by ducking old room/casing energy, then applies a linked limiter.

The models are deliberately perceptual and low-order rather than measurement-grade solvers.
