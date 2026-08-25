# expandiblefire89

Small C89/fixed-point projectile-behaviour provider for Blank3D's vendored
Weapon System.

It tracks actual travelled distance, linearly expands a projectile between a
start/end scale, and marks it dead at an optional kill distance. It does not
own projectile pools, rendering, collision, damage, actors, weapons or memory.
The host may apply the returned scale to both visual geometry and collision
radius, making the low-poly primitive the gameplay truth while allowing later
billboards/textures to remain presentation-only.

Protocol: signed Q20.12 (`4096 == 1.0`). Caller-owned state only; no heap and no
floating point.
