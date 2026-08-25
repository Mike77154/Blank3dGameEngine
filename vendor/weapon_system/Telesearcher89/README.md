# Telesearcher89

Provider-driven projectile `move_to_target` / homing steering.

Each tick the caller supplies projectile position/velocity. The host target
provider resolves or reacquires an actor position; Telesearcher89 returns a
new velocity aimed toward that target while preserving the requested speed.
A fixed-point gain permits snap-homing or softer steering.

- C89
- integer/fixed-point only
- no malloc/calloc/realloc/free
- no ownership of projectile pools or actors
- no OS dependency
