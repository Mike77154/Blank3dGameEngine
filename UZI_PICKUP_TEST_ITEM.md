# Uzi / SMG pickup test item

This package adds a deliberately non-starting weapon and matching ammunition
as real world pickups. The content is intended to exercise the integrated
CT89 -> PBB -> GKInventory -> GWP89 path from the ordinary Blank3D scene.

## Scene authoring

`scripts/startup.rpy` contains:

    weapon_pickup config/pickups/uzi_weapon.ini pos -6.5 0 24
    ammo_pickup   config/pickups/uzi_ammo.ini   pos -9.5 0 24

Change those RPY coordinates to reposition the objects. The RPY command does
not contain weapon/ammo IDs or mesh dimensions; those remain data-driven.

The generic command is also registered:

    pickup <pickup.ini> pos <x> <y> <z>

`weapon_pickup` and `ammo_pickup` are typed aliases that force the expected
kind while still reading the resource and presentation data from the INI.

## Content files

- `config/weapons/uzi.ini`: GWP89 weapon profile, weapon id 11, automatic.
- `config/weapons/weapons.ini`: catalog entry for id 11 / ammo id 10.
- `config/weapons/player_weapons.ini`: intentionally has NO weapon 11 and NO
  ammo 10 starting grants.
- `config/pickups/uzi_weapon.ini`: PBB/CT89 weapon pickup definition.
- `config/pickups/uzi_ammo.ini`: PBB/CT89 ammo pickup, amount 64.
- `config/pickups/uzi_weapon_mesh.ini`: two-box world mesh recipe.
- `config/pickups/uzi_ammo_mesh.ini`: magazine-body + cap two-box recipe.
- `objects/pickup_item.gfo`: shared Object/GFO lifecycle for world pickups.

## World mesh recipes

The weapon pickup is intentionally primitive and unmistakable during testing:

    receiver  = rectangular box
        +
    grip/mag  = rectangular box

The ammunition pickup is:

    cap/feed block = rectangular box
          +
    magazine body = rectangular box

All dimensions and colors are fixed-point/text data in the recipe INIs. The
runtime allocates no heap memory for these meshes.

## Ownership/lifecycle

A pickup is a world Object/Thing/ECS instance, but it is not an Actor. It has:

- a host subject id for world identity;
- a CT89 trigger for contact detection;
- a PBB item for rules/effects/lifecycle;
- a GFO Object instance for Object lifecycle;
- a recipe mesh for rendering.

On successful contact, PBB asks the weapon bridge to grant the resource. Only
if GKInventory/GWP89 accept it is the item consumed. The engine then removes
its visual mesh and retires the corresponding GFO/Thing/ECS runtime instance.
If a weapon pickup is rejected (for example max_owned=1 is already reached),
it remains in the world.

## Test behavior

1. Start the scene: player has no weapon 11.
2. Walk to `(4, 0, 4)` and touch the two-box Uzi.
3. Weapon 11 is granted and auto-equipped; the world Uzi disappears.
4. Walk to `(7, 0, 4)` and touch the magazine/cap object.
5. 64 units of ammo type 10 are granted; the ammo pickup disappears.

Run `make test-uzi-pickup` for the portable content + GFO regression.
