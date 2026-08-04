# Blank3D v3.15.2 — Ground Lancer horizontal dash fix

## Root cause

`ground_lancer_enemy` was initialized with `GGL_STYLE_PEGASUS` and a 0.35-unit
triangle-wave hop. The requested knight charge was therefore receiving a
vertical oscillation on every charge frame.

## Fix

- `ground_lancer_enemy` now defaults to `GGL_STYLE_LANCE`.
- Hop height is forced to zero for the archetype and in its FPIL setup.
- Lance/RAM movement is clamped to the floor plane.
- STOP contacts end on the configured contact-radius shell, preventing the
  final full frame from overlapping or passing through the player.
- Pegasus and skim-hop remain available as explicit opt-in styles.

## Regression coverage

`tests/test_motion_attacks.c` verifies that the Ground Lancer:

1. uses lance style,
2. never changes Y,
3. reaches an impact,
4. finishes, and
5. remains one contact radius from the target.
