# Projectile forward-hemisphere fix v3.6.4

The HUD ray, Cameranaku view and physical muzzle now share a launch invariant:

```text
dot(projectile_direction, rendered_camera_forward) > 0
```

If a provider-axis mismatch produces a target behind the visible camera, the
target is rebuilt at the same range on the rendered camera ray. The projectile
is then re-aimed from the physical muzzle to that point before gravity, Bolt3D,
spread, collision and trails are applied.

This does not disable or bypass `CNK_TRANSFORM_MODE_RECEIVE_PROVIDER`, and it
does not alter the four mouse feeder negations. The correction lives only at
the camera-to-projectile launch boundary.
