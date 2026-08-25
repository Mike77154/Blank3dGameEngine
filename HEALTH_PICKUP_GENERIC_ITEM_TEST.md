# Health pickup generic-item proof

This test deliberately uses the generic RPY command:

```text
pickup "config/pickups/health_box.ini" pos -4.5 0 24
```

It does **not** use `weapon_pickup` or `ammo_pickup`.

Pipeline:

```text
RPYL pickup
  -> Blank3DPickupWorld
  -> pickup_item.gfo / Thing
  -> CT89 touch trigger
  -> PBB rule: player health < 100
  -> PBB custom host effect: add item amount to Blank3D player health
  -> PBB consume item
  -> GFO/mesh lifecycle disappears with the consumed item
```

The white box has `amount=25`. At 100 HP the PBB rule rejects the touch, so
the box stays. After the player is hurt, leaving and re-entering the trigger
heals up to 25 HP (clamped to 100) and consumes the box.

The health item has no weapon resource id and no weapon/ammo pickup binding.
The weapon bridge still supplies the CT89-to-PBB routing, but its weapon grant
gate sees no weapon pickup binding and simply passes the generic PBB item
through. This verifies that the world pickup handler is not weapon-only.
