# Blank3D item/contact/weapon integration

This integration vendors three independent layers and joins them only through
providers:

```text
host spatial query / collision
        |
        v
3D_contact_trigger89
  ENTER / ACTIVATE
        |
        v
pbb_contact_weapon_bridge89
        |
        v
pbb_item_system_c89
  rules -> action gate -> effects/consume
              |
              v
       Blank3D weapon provider
       GKInventory + GWP89
```

## What owns what

- `3D_contact_trigger89` owns contact/proximity/manual trigger state and events.
- `pbb_item_system_c89` owns item definitions, item instances, actor masks,
  rules, effects and item lifecycle.
- `pbb_contact_weapon_bridge89` only translates identities and transactions.
- Blank3D/GKInventory remains the authoritative weapon/ammo inventory.
- GWP89 remains the authoritative weapon runtime/equip/fire system.
- Collision is still external: install a `CT89_SensorProvider` on
  `blank3d_systems_contact_triggers()`.

No module owns heap memory. All bridge tables use fixed compile-time capacity.

## Transaction rule

PBB has an optional action gate. For a weapon/ammo pickup the sequence is:

1. PBB validates touch/interact rules.
2. The bridge asks the Blank3D weapon provider to grant the resource.
3. If the host rejects the grant (capacity, invalid id, etc.), PBB returns
   blocked and does not apply the consume effect.
4. CT89 receives `CT89_ACTION_REJECTED`, so its consume policy is not run.
5. If accepted, PBB applies effects/consume and CT89 applies its trigger consume
   policy.

This prevents a full inventory from deleting a pickup that was not actually
received.

## Frame integration

`blank3d_systems_update()` calls `ct89_step()` once per frame. With no spatial
provider attached this is a no-op. Once a provider is installed, TOUCH and
PROXIMITY triggers participate in the normal systems update loop.

For explicit use/interact actions call:

```c
blank3d_systems_activate_item_trigger(&systems, trigger, player_subject);
```

A non-manual trigger can only be activated while the activator is currently in
its contact/proximity relation. A `CT89_SENSOR_MANUAL` trigger can be activated
directly.

## High-level Blank3D helpers

```c
blank3d_systems_define_weapon_pickup(...);
blank3d_systems_define_ammo_pickup(...);
```

They create the logical PBB item, attach a consume effect, bind the resource
grant, and create the CT89 trigger. `interact_required == 0` creates a touch
pickup; nonzero creates an interact pickup.

The item position inside PBB is deliberately not used as Blank3D's 3D collision
source. The host's CT89 spatial provider is authoritative for 3D placement. This
keeps PBB usable in 2D/3D/non-spatial hosts and avoids duplicating physics.

## Lower-level access

Blank3D exposes the vendored systems when custom behavior is needed:

```c
CT89_Context *ct = blank3d_systems_contact_triggers(&systems);
PBB_ItemWorld *items = blank3d_systems_item_world(&systems);
PBBCTW89_Bridge *bridge = blank3d_systems_item_contact_bridge(&systems);
```

Additional actors can be mapped with `blank3d_systems_bind_item_actor()`.
The generic bridge itself has no Blank3D includes, so it can be reused by other
hosts with a different inventory/weapon implementation.

## Validation

Portable bridge regression:

```sh
make test-item-contact-weapon
```

It covers:

- touch weapon pickup accepted;
- rejected weapon grant leaves item and trigger alive;
- manual interact ammo pickup;
- manual touch-semantic weapon pickup.

The original upstream PBB and Contact Trigger tests remain usable unchanged.
