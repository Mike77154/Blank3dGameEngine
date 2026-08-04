# Blank3D v3.25.1 — GFaction89 Allied Targets

## Purpose

Blank3D no longer treats every non-player actor as an enemy and no longer
assumes that every AI target is the player. `gfaction89` is vendored under
`vendor/gfaction89/` and acts as the deterministic relationship and target
legality layer.

The library remains C89, fixed-point, static-capacity and heap-free. Its
resolution order is preserved:

1. default relationship;
2. faction matrix;
3. same-team fallback;
4. role rules;
5. tag rules;
6. per-entity overrides.

## Runtime flow

```text
Socketer
  knows the position and identity of every live actor
        |
        v
GFaction89
  filters legal candidates and scores interest
  from faction/team/role/tags, threat, distance,
  visibility, hearing and recent damage
        |
        v
NPC Eyes + EnlightenerAI
  describe confirmation and stimulus quality
        |
        v
GEDER
  accumulates absolute and augmented truths
        |
        v
FPIL
  uses targetalive/canattacktarget/rotatetotarget/firetarget
```

Socketer remains the omniscient truth base. GFaction does not discover actor
positions; it answers what a source actor is permitted or encouraged to do
with each known candidate.

## Player is an ordinary faction entity

At scene load the player is registered as:

```text
faction = player
team    = survivors
role    = hero
tags    = human,organic,armed,player,targetable
```

Actors spawned through RPYL can override the same fields:

```text
armed_ally 7 pos -4 0 8 hp 60 \
  loadout config/npc_loadouts/armed_ally.ini \
  faction allies team survivors role armed_ally \
  tags human,organic,armed,ally,targetable
```

The archetype supplies convenient defaults, but the relationship is data.
Any other actor can use the same `faction`, `team`, `role` and `tags` pairs.

## Faction graph

`config/factions/gameplay.ini` defines these initial groups:

```text
player   <-> allies   : assist, protect, follow
player   -> hostile   : attack
allies   -> hostile   : attack
hostile  -> player    : attack
hostile  -> allies    : attack
hostile  -> hostile   : ignore
neutral  -> survivors : ignore
neutral  -> hostile   : fear/flee
```

The vendored library still supports tag rules, role rules, entity overrides
and declarative event triggers. Blank3D dispatches damage/death events through
`gfa_fire_event`, so future faction conversions or mission reactions can be
added in the INI without changing the combat code.

## Armed ally proof actor

The scene now spawns `armed_ally` at actor slot 7.

Visual composition:

```text
blue capsule body
+ dark-blue box head
+ universal equipped weapon object
```

Weapon flow:

```text
GWeapon89 loadout
  -> GAttach weapon_r attachment
  -> NationalMecanicanimal89 mechanical rig
  -> muzzle socket / projectile systems
```

The exact same equipment path is used by hostile gunners. Actor faction does
not change how the object is attached or animated.

## Generic targeting and damage

Each live actor is offered as a target candidate. `gfa_eval` decides whether
it may be attacked and returns a fixed-point desirability score. The selected
actor ID is then published to the actor's perception agent.

Important behavior:

- allies never select the player or other allies as attack targets;
- allies select hostile actors and use generic target-first FPIL verbs;
- hostiles may select the player or an ally;
- the actor that recently damaged a hostile receives a temporary interest
  bonus, allowing the hostile to retarget the armed ally;
- bullets, rockets, explosions and scripted melee all use the same GFaction
  legality gate;
- friendly bodies may physically block projectiles, but friendly-fire damage
  is rejected unless the relationship data explicitly enables it;
- target changes clear target-specific Eyes/Enlightener memory so facts from
  one actor are not inherited by another.

## New FPIL relationship conditions

```text
targethostile
canattacktarget
targetisplayer
selfisally
```

Existing player-oriented aliases remain accepted, but canonical behavior uses:

```text
targetalive
rotatetotarget
movetotarget
firetarget
hurttarget
```

## Files

```text
vendor/gfaction89/
src/blank3d_faction.h
src/blank3d_faction.c
config/factions/gameplay.ini
config/entities/armed_ally.ini
config/npc_loadouts/armed_ally.ini
objects/armed_ally.gfo
scripts/armed_ally.fpi
scripts/startup.rpy
tests/test_faction_bridge.c
```

## Build and test

```bash
make test-gfaction-vendor
make test-faction-bridge
make test-languages
make test-gfo-entities
make test-perception-stack
make test-actor-equipment
make test-collision
make syntax-check
make audit
```
