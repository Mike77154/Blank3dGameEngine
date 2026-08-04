# gfaction89 INI reference

## Global lists

```ini
[factions]
0 = player
1 = rpd
2 = civilian
3 = undead

[teams]
0 = stars
1 = outbreak

[roles]
0 = hero
1 = infected
2 = noncombatant

[tags]
0 = human
1 = organic
2 = infected
3 = machine
```

The numeric key is decorative. Registration order follows the file order.

## Entity

```ini
[entity:30]
faction = undead
team = outbreak
role = infected
tags = infected,organic
threat = 55
morale = 0
alive = yes
targetable = yes
```

## Relation

```ini
[relation:undead:player]
disposition = hate
priority = 100
flags = attack,call_help
score_bias = 0
```

## Tag rule

```ini
[tag_rule:infected:organic]
disposition = prey
priority = 40
flags = attack
```

Use `*` or `any` as wildcard:

```ini
[tag_rule:machine:*]
disposition = ignore
priority = 10
flags = ignore
```

## Role rule

```ini
[role_rule:guard:noncombatant]
disposition = protect
priority = 70
flags = protect,assist
```

## Override

```ini
[override:30:1]
disposition = ignore
priority = 999
flags = ignore,scripted
```

## Trigger

```ini
[trigger:zombie_kills_civilian_convert]
event = death
src_faction = undead
dst_faction = civilian
dst_tag = human
set_dst_faction = undead
set_dst_team = outbreak
set_dst_role = infected
add_dst_tag = infected
clear_dst_tag = human
mark_dst_alive = 1
```

Supported event names:

- death
- damage
- sense
- touch
- script
- any

Supported condition fields:

- src_faction, dst_faction
- src_team, dst_team
- src_role, dst_role
- src_tag, dst_tag
- required_disposition

Supported action fields:

- set_src_faction, set_dst_faction
- set_src_team, set_dst_team
- set_src_role, set_dst_role
- add_src_tag, add_dst_tag
- clear_src_tag, clear_dst_tag
- mark_src_alive, mark_dst_alive
