# gfaction89 v1

C89 faction/tag relationship resolver for game AI.

This is not an AI brain. It is a small, deterministic decision filter that any AI, behavior tree, finite state machine, script VM, sensor system, or entity controller can call.

The core query is:

```c
GFA_Decision d = gfa_eval(&world, actor_id, candidate_id, &ctx);
```

The decision answers:

- disposition: neutral, ally, hate, fear, prey, protect, contain, avoid, etc.
- flags: attack, assist, flee, protect, follow, call_help, convert, scripted.
- priority: rule priority.
- score: fixed-point target desirability.

## Design rules

- C89 compatible.
- No heap allocation in the library.
- No realloc/free path.
- No float/double.
- Fixed-point scoring.
- Static capacity tables.
- Optional user scratch arena.
- Fully INI-exposed.
- 2D/3D agnostic.
- Player can be just another entity with a faction.
- Any entity can change faction/team/role/tags at runtime.
- Hooks allow external systems to react to faction changes and triggers.

## Build

```sh
make
./demo_zombie_rampage
./demo_faction_war
```

On MinGW/MSYS2:

```sh
gcc -std=c89 -Wall -Wextra -pedantic -Iinclude src/*.c demo/demo_zombie_rampage.c -o demo_zombie_rampage.exe
```

## Minimal use

```c
GFA_World world;
GFA_Context ctx;
GFA_Decision d;
int undead;
int player;

gfa_init(&world);
undead = gfa_register_faction(&world, "undead");
player = gfa_register_faction(&world, "player");

gfa_set_relation(&world, undead, player, GFA_DISP_HATE, 100, 0, 0);

gfa_set_entity_faction(&world, 30, undead);
gfa_set_entity_faction(&world, 1, player);

ctx.stimulus = GFA_STIM_SIGHT;
ctx.distance_fp = GFA_FP_FROM_INT(5);
ctx.visible = 1;
ctx.heard = 0;
ctx.recent_damage = 0;
ctx.mission_bias = 0;

d = gfa_eval(&world, 30, 1, &ctx);
if (d.can_attack) {
    /* AI may attack entity 1. */
}
```

## Layer order

The resolver starts with defaults and then applies stronger rules:

1. Defaults.
2. Faction matrix.
3. Same team fallback.
4. Role rules.
5. Tag rules.
6. Per-entity overrides.

The higher priority rule wins. Overrides are normally given very high priorities for cutscenes or mission logic.

## Runtime conversion example

The demo contains a fully INI-driven trigger:

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

When source entity from faction `undead` kills a target from faction `civilian`, the target respawns as undead and gains the `infected` tag.

## Integration pattern

A generic AI loop should do this:

```txt
perception/sensors -> candidate ids -> gfaction89 -> AI behavior
```

The AI remains agnostic. It only sees a decision such as attack, flee, protect, assist, or ignore.
