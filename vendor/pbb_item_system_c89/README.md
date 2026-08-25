# PBB Item System C89 v0.2

Standalone **item interaction system** for C89 engines.

This module is intentionally **not an inventory system**. It handles:

```txt
actor touches / interacts / drops item
    -> validate rules
    -> apply effects
    -> mutate actor/world/item variables or flags
    -> hide, consume, destroy, spawn, recycle, or drop items
    -> emit fixed-size events
```

## v0.2 goals

- Keep the v0.1 public API usable through `#include "pbb_item_system.h"`.
- Split the old monolithic `.h/.c` into focused public headers and focused source modules.
- Add item category masks and actor touch/interact masks.
- Add optional strict manual-droppable checks.
- Add consumed-item GC and optional auto-recycle-on-consume.
- Add renderer/query-friendly item iterators.
- Add compact runtime save/load helpers.

## Constraints

- C89 style.
- No heap ownership inside the module.
- No `malloc`, `free`, or `realloc` calls.
- No `float` or `double` in the core API.
- Positions and sizes use signed Q16.16 fixed point via `PBB_Fixed`.
- All storage lives inside `PBB_ItemWorld` fixed arrays.
- Capacities are compile-time macros.

## File layout

```txt
pbb_item_system_c89_02_modular_final/
├─ include/
│  ├─ pbb_item_config.h     # version, capacities, fixed point
│  ├─ pbb_item_types.h      # public enums, flags, structs
│  ├─ pbb_item_world.h      # world, actor, def, rule/effect registry API
│  ├─ pbb_item_runtime.h    # spawn, lifecycle, touch/interact/drop API
│  ├─ pbb_item_events.h     # event queue API
│  ├─ pbb_item_rules.h      # rule block evaluation API
│  ├─ pbb_item_effects.h    # effect block application API
│  ├─ pbb_item_drops.h      # drop table API
│  ├─ pbb_item_iter.h       # item iterator API
│  ├─ pbb_item_save.h       # runtime save/load API
│  └─ pbb_item_system.h     # umbrella include
├─ src/
│  ├─ pbb_item_internal.h
│  ├─ pbb_item_internal.c   # internal helpers
│  ├─ pbb_item_world.c      # world init/options/rng/world vars
│  ├─ pbb_item_actor.c      # actor lifecycle, masks, vars, flags
│  ├─ pbb_item_def.c        # defs, rules, effects registration
│  ├─ pbb_item_items.c      # item lifecycle, spawn, counts, GC
│  ├─ pbb_item_interact.c   # touch/interact logic
│  ├─ pbb_item_drop.c       # manual drop logic
│  ├─ pbb_item_rules.c      # built-in rule evaluation
│  ├─ pbb_item_effects.c    # built-in effects
│  ├─ pbb_item_events.c     # event ring buffer
│  ├─ pbb_item_drops.c      # drop tables
│  ├─ pbb_item_iter.c       # item iterators
│  └─ pbb_item_save.c       # runtime save/load
├─ demo/
│  └─ pbb_item_demo.c
├─ tests/
│  └─ pbb_item_tests.c
├─ Makefile
└─ README.md
```

## Build

```sh
make test
make demo
make lib
```

Run:

```sh
./build/pbb_item_tests
./build/pbb_item_demo
```

Clean:

```sh
make clean
```

The default flags are:

```sh
-std=c89 -Wall -Wextra -pedantic -Iinclude
```

## Basic usage

```c
#include "pbb_item_system.h"

PBB_ItemWorld world;
int player;
int coin_def;
int first_effect;

pbb_item_world_init(&world);

player = pbb_item_actor_create(&world,
                               PBB_ACTOR_CLASS_PLAYER,
                               1UL,
                               PBB_FIXED_FROM_INT(10),
                               PBB_FIXED_FROM_INT(10),
                               PBB_FIXED_HALF,
                               PBB_FIXED_HALF);

coin_def = pbb_item_def_create(&world, "coin");
pbb_item_def_set_defaults(&world,
                          coin_def,
                          PBB_ITEMF_ACTIVE | PBB_ITEMF_VISIBLE | PBB_ITEMF_TOUCHABLE,
                          1,
                          PBB_FIXED_HALF,
                          PBB_FIXED_HALF);
pbb_item_def_set_category(&world, coin_def, PBB_ITEM_CATEGORY_PICKUP);

first_effect = pbb_item_effect_add(&world,
                                   PBB_EFFECT_ADD_ACTOR_VAR_FROM_ITEM_AMOUNT,
                                   1,  /* actor var id: money */
                                   1,  /* amount multiplier */
                                   0,
                                   0,
                                   0,
                                   0);

pbb_item_effect_add(&world, PBB_EFFECT_CONSUME_ITEM, 0, 0, 0, 0, 0, 0);
pbb_item_def_set_effect_block(&world, coin_def, PBB_ITEM_HOOK_TOUCH, first_effect, 2);

pbb_item_spawn(&world,
               coin_def,
               PBB_FIXED_FROM_INT(10),
               PBB_FIXED_FROM_INT(10),
               5);

pbb_item_touch_actor_all(&world, player);
```

## v0.2 category masks

Each item definition has a default category mask:

```c
pbb_item_def_set_category(&world, coin_def, PBB_ITEM_CATEGORY_PICKUP);
pbb_item_def_set_category(&world, switch_def, PBB_ITEM_CATEGORY_SWITCH);
pbb_item_def_set_category(&world, trap_def, PBB_ITEM_CATEGORY_HAZARD);
```

Each actor has touch/interact masks:

```c
pbb_item_actor_set_masks(&world,
                         player,
                         PBB_ITEM_CATEGORY_PICKUP | PBB_ITEM_CATEGORY_HAZARD,
                         PBB_ITEM_CATEGORY_SWITCH | PBB_ITEM_CATEGORY_KEY);
```

A touch/interact is blocked before rules/effects when:

```txt
(actor mask & item category) == 0
```

Blocked mask checks emit `PBB_EVENT_ITEM_RULE_BLOCKED` with `PBB_ITEM_BLOCK_REASON_MASK` in event field `a`.

## v0.2 strict manual drop mode

Strict mode is opt-in so old v0.1 behavior remains compatible by default:

```c
pbb_item_world_set_strict_droppable(&world, 1);
```

When strict mode is on, `pbb_item_drop_from_actor()` and `pbb_item_drop_at()` refuse definitions that do not include `PBB_ITEMF_DROPPABLE` in their default flags. Failed strict drops emit `PBB_EVENT_ITEM_RULE_BLOCKED` with `PBB_ITEM_BLOCK_REASON_NOT_DROPPABLE`.

Drop tables are not affected by strict manual-drop mode; they represent loot spawning, not actor-owned manual dropping.

## v0.2 consumed item recycling

By default, consumed items stay allocated but inactive. This preserves v0.1 save/history behavior.

Manual cleanup:

```c
int freed = pbb_item_gc_consumed(&world);
```

Global auto-recycle:

```c
pbb_item_world_set_recycle_consumed(&world, 1);
```

Per-item/definition auto-recycle:

```c
pbb_item_def_set_defaults(&world,
                          coin_def,
                          PBB_ITEMF_ACTIVE |
                          PBB_ITEMF_VISIBLE |
                          PBB_ITEMF_TOUCHABLE |
                          PBB_ITEMF_RECYCLE_ON_CONSUME,
                          1,
                          PBB_FIXED_HALF,
                          PBB_FIXED_HALF);
```

`PBB_ITEMF_PERSISTENT` prevents auto-recycle.

## v0.2 iterators

For rendering or gameplay queries:

```c
PBB_ItemIter it;
PBB_Item *item;

pbb_item_iter_begin(&it,
                    PBB_ITEMF_VISIBLE,
                    0UL,
                    PBB_ITEM_STATE_WORLD,
                    PBB_ITEM_CATEGORY_PICKUP);

while (pbb_item_iter_next(&world, &it, &item)) {
    /* draw or query item */
}
```

Convenience initializers:

```c
pbb_item_iter_visible_begin(&it);
pbb_item_iter_active_begin(&it);
```

## v0.2 runtime save/load

Definitions/rules/effects/drop tables are treated as game database data. `PBB_ItemSaveState` stores runtime state:

```txt
actors[]
items[]
world_vars[]
world_flags[]
rng_state
strict/recycle runtime options
```

Usage:

```c
PBB_ItemSaveState save;

pbb_item_save_runtime(&world, &save);
/* ... mutate runtime ... */
pbb_item_load_runtime(&world, &save);
```

Loading resets the event queue and preserves callbacks, definitions, rules, effects, and drop tables already present in `PBB_ItemWorld`.

## Capacity tuning

Override before including headers or in compiler flags:

```c
#define PBB_ITEM_MAX_ACTORS 64
#define PBB_ITEM_MAX_ITEMS 256
#define PBB_ITEM_MAX_DEFS 128
#define PBB_ITEM_MAX_RULES 256
#define PBB_ITEM_MAX_EFFECTS 512
#define PBB_ITEM_MAX_EVENTS 256
#define PBB_ITEM_MAX_ACTOR_VARS 64
#define PBB_ITEM_MAX_WORLD_VARS 128
#define PBB_ITEM_FLAG_WORDS 4
#define PBB_ITEM_CUSTOM_COUNT 4
```

Example:

```sh
gcc -std=c89 -DPBB_ITEM_MAX_ITEMS=1024 -Iinclude src/*.c demo/pbb_item_demo.c
```

## Notes

- Item IDs are fixed-array indices.
- Definition IDs are fixed-array indices.
- `pbb_item_system.h` remains the easiest include for users.
- Focused headers are available for engines that prefer narrower includes.
- Custom rules/effects still use `PBB_RULE_CALL_CUSTOM` and `PBB_EFFECT_CALL_CUSTOM`.
- No heap allocation was added in v0.2.

## Optional pre-effect action gate (Blank3D-compatible extension)

`pbb_item_world_set_action_gate()` installs a host callback that runs after the
normal PBB rules pass but before TOUCH/INTERACT effects mutate the item/world.
Returning zero blocks the action and emits `PBB_EVENT_ITEM_RULE_BLOCKED` with
`PBB_ITEM_BLOCK_REASON_PROVIDER`. With no gate installed, v0.2 behavior is
unchanged.

This is useful for transactional pickups: an external inventory can reject an
item before PBB consumes it.
