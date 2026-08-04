# Ballistic catalog, spread, Gatling and Magnum fix — v3.3

## Corrected contracts

- `gbulletmesh89` is now exposed as two explicit catalogs: projectile meshes and
  casing meshes. `projectile_mesh_id` never selects brass, and `shell_mesh_id` never
  selects a flying projectile.
- Pistol, machine gun, shotgun, Magnum and sniper use their matching shell builders.
- Gatling temporarily uses machine-gun projectile id/mesh id `2` and machine-gun
  shell mesh id `2`, while keeping weapon id `8` and its independent belt ammo.
- A host-side recovery contract spawns the Gatling projectile from `FIRE_ACCEPTED`
  only when a custom/saturated event bus dropped the normal projectile request.
  Normal event flow does not duplicate bullets.
- Shotgun spread now treats profile values as degrees. The bounded small-angle cone
  preserves forward as the dominant direction instead of producing cardinal rays.
- Magnum is present in N/M cycling and direct key `4`.

## Weapon cycle

```text
pistol -> shotgun -> machine gun -> Magnum -> Gatling -> sniper
       -> grenade launcher -> rocket launcher -> wrap
```

Direct keys are `1` through `8` in that same order.

## Automated checks

- Seven shotgun projectile events remain inside a forward cone.
- Gatling consumes one belt round and emits projectile id/mesh id `2`.
- Gatling casing request uses shell mesh id `2`.
- Magnum is reachable between machine gun and Gatling.
- Five casing meshes and eight projectile meshes build from the specialized libraries.
