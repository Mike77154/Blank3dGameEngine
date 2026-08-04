# Sniper mask and firearm mechanism synchronization — v3.5

## Circular scope mask

The scope mask no longer paints only four rectangles around the lens bounding
square. The renderer now fills the complete complement of the circular lens:

```text
full viewport
  - circular visible lens
  = one continuous opaque outside mask
```

The implementation uses two side rectangles and two 128-segment curved quad
strips. It requires no stencil buffer, floating-point simulation or dynamic
allocation. The outer region uses one RGBA value (`0x000000F0`) and the lens
outline is drawn after the mask.

## Event-synchronized firearm synthesis

Weapon audio is driven by the same typed events that create gameplay effects.
The slingshot is deliberately excluded until an elastic/string synthesis family
is added.

```text
GWP89_FIRE_ACCEPTED
  -> firearm DNA/profile
  -> muzzle device/profile gas
  -> receiver/body report
  -> family action mechanism
  -> ammunition/feed motion
  -> shot report

GWP89_PROJECTILE_REQUEST
  -> projectile family/flyby voice
  -> RPG whistle + spin lifetime handles

GWP89_CASING_REQUEST
  -> exact casing family/contact voice

GWP89_RELOAD_BEGIN / END
  -> magazine remove/insert/seat where applicable
  -> loose shell, tube, cylinder or belt movement
  -> chamber/pump/bolt/feed action

collision / projectile lifetime
  -> impact, ricochet or ordnance blast
  -> release continuous projectile voices
```

## Firearm family map

| Weapon | DNA/report family | Action | Feed | Casing | Projectile/ordnance |
|---|---|---|---|---|---|
| Pistol | service pistol | pistol slide | box magazine | pistol brass | near/supersonic bullet |
| Machine gun | SMG | automatic machine | steel box magazine | steel case | tracer/supersonic bullet |
| Shotgun | shotgun | pump shotgun | tube | plastic shell | pellet swarm |
| Magnum | magnum | revolver | loose rounds | magnum brass | heavy supersonic bullet |
| Sniper | sniper | rifle/bolt | sniper box | rifle brass | supersonic N-wave |
| Grenade launcher | launcher | pump/breech | loose 40 mm | large shell family | tracer + grenade blast |
| Rocket launcher | launcher | launcher/rifle action | loose rocket | none | RPG whistle, spin and blast |
| Gatling | heavy | machine/rotor | belt box | rifle/MG brass | MG tracer + Gatling motor/feed |

## Projectile lifetime ownership

Each active engine projectile now owns optional synthesis keys. RPG flight
voices start when the projectile is spawned, receive motion updates while it is
alive, and release when the projectile impacts, expires or is recycled. This
prevents a fixed sound duration from drifting away from the actual animation or
ballistic lifetime.
