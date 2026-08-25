# Satellaborner89

Provider-driven projectile spawn relocation.

It asks the host for a target actor position and rewrites a projectile's spawn
origin to that position plus an optional fixed-point offset. It does not own
actors, targeting, collision, rendering, or projectile pools.

Designed for effects such as satellite strikes, target-originating attacks,
telefrag-style projectiles, rain-from-above emitters, and other weapons whose
payload is born at/near the target rather than at the shooter's muzzle.

- C89
- integer/fixed-point only
- no malloc/calloc/realloc/free
- caller/host-owned state
- no OS dependency
