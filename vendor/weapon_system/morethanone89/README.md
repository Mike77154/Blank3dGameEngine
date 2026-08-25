# morethanone89

Small C89, fixed-capacity selector for charge/range-based projectile variants.

The library does **not** own input, charging, weapons, ammo, rendering, physics,
or projectiles. The host supplies a duration in milliseconds and receives the
highest configured stage whose `min_ms` threshold was reached.

Blank3D uses it for Buster-style weapons: a normal press fires the base shot;
a sufficiently long hold activates `morethanone89`; release fires the selected
charged projectile stage.

Constraints: C89, no dynamic allocation, no float/double, maximum 8 stages.
