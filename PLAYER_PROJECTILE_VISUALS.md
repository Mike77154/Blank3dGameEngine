# Player projectile visuals — v3.26.1

## Goal

Keep the player camera ray authoritative for damage while making the visual
projectile and its own trail visible. The complete yellow hitscan/debug segment
must never be presented to the player.

## Separation

```text
camera/muzzle raycast
  -> collision + damage only
  -> never submitted to rendering

resolved visual origin -> impact point
  -> short-lived cosmetic projectile mesh
  -> Aoi Trail3D profile
  -> no collision, no damage, no NPC routing
```

The visual projectile begins at the weapon muzzle when one is available. In
FPS it still uses the camera ray for gameplay authority, but its presentation
may start at the muzzle so the projectile does not appear inside the camera.

## Timing

The cosmetic projectile reaches the already-resolved impact in a short fixed
presentation window:

- Gatling: 72 ms
- Machine gun: 80 ms
- Sniper: 64 ms
- Shotgun: 88 ms
- Other linear player weapons: 96 ms

It remains alive briefly after arrival so the trail can be seen and then the
slot is released. It never sweeps collision and never applies damage.

## Rendering contract

- Cosmetic projectile meshes are rendered like ordinary projectile meshes.
- Cosmetic projectiles attach the configured Aoi Trail3D profile.
- If the trail provider is full, only the tiny previous-to-current movement
  segment is drawn in pale blue as a fallback.
- The full origin-to-impact authority ray is never drawn.
- NPC projectiles and physical grenade/rocket/Bolt3D paths are unchanged.
