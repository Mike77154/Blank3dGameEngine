# Blank3D v3.6.6 - TPS shotgun spread regression fix

## Regression

The v3.6.5 third-person zero-plane repair shortened every empty-space target
onto `camera_forward`. The weapon manager had already applied a different ray
for each shotgun pellet, but the runner collapsed those rays back into one.

## Fix

For an empty-space TPS event, `blank3d_ballistics_align_event_to_view()` now:

1. derives the pellet ray from `camera_origin -> event.hit_point`;
2. validates that it remains in the visible camera hemisphere;
3. shortens that individual ray to the stable 32-unit TPS zero plane;
4. computes the physical launch direction from muzzle to that pellet target.

A real hit remains exact. An unreachable close hit still falls back to the
center camera ray to avoid a near-vertical launch.

This keeps the shotgun pattern centered around the HUD while preserving the
individual spread of all seven pellets.
