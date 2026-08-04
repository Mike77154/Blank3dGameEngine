# gfaction89 design

## Purpose

`gfaction89` is a faction and relationship resolver. It separates target legality from AI behavior.

Bad pattern:

```txt
zombie AI = search player
```

Good pattern:

```txt
zombie AI = perceive candidates, ask relationship filter, act on returned permission
```

This allows the same zombie AI to attack the player, attack civilians, ignore other zombies, fight soldiers, chase animals, or obey a scripted override without hardcoded names.

## Entities

Each entity may have:

- faction: broad allegiance.
- team: smaller squad or temporary alliance.
- role: semantic behavior role, such as hero, noncombatant, infected, turret, medic.
- tags: bitmask properties, such as human, organic, machine, infected, armed, unarmed.
- threat/morale: integer stats used by scoring.

The player is just an entity. A player can be `player`, `rpd`, `undead`, `umbrella`, or any runtime faction.

## Dispositions

- ignore
- neutral
- ally
- friendly
- hate
- fear
- prey
- protect
- rival
- contain
- avoid
- owner
- scripted

## Flags

- attack
- assist
- flee
- protect
- friendly_fire
- ignore
- follow
- contain
- call_help
- convert
- scripted

## Rule layers

### Faction matrix

Best for broad faction wars.

```ini
[relation:rpd:undead]
disposition = hate
priority = 100
flags = attack
```

### Tag rules

Best for physical/logical properties.

```ini
[tag_rule:infected:organic]
disposition = prey
priority = 40
flags = attack
```

### Role rules

Best for game design roles.

```ini
[role_rule:guard:noncombatant]
disposition = protect
priority = 70
flags = protect,assist
```

### Overrides

Best for cutscenes and special mission logic.

```ini
[override:30:1]
disposition = ignore
priority = 999
flags = ignore,scripted
```

## Trigger model

Triggers are not full scripting. They are declarative conditional actions for common relationship changes.

A game script can still call:

```c
gfa_set_entity_faction(&world, entity_id, new_faction);
gfa_add_entity_tag(&world, entity_id, tag_id);
gfa_fire_event(&world, &event_data);
```

Use triggers for simple transformations and script callbacks for complex quests.
