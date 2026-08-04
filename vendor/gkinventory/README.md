# GKINV C89 Inventory Core

A small, engine-agnostic inventory and item-management core written for strict C89/C90-style projects.

## Goals

- C89/C90-friendly public API.
- No calls to `malloc`, `calloc`, `realloc`, or `free`.
- No C99 headers such as `stdint.h` or `stdbool.h`.
- No floating-point numeric types in the core.
- User-provided static buffers only.
- Integer Q16.16 fixed-point helpers for weight/cost-like values.
- Generic containers: player inventory, storage box, chests, shops, loot, hotbars, equipment slots, crafting inputs.
- Hooks for gameplay logic: accept/reject, use, events.
- Optional grid inventory support.
- Optional recipe/combine support.
- Optional text save/load through caller-provided writer callbacks.

## Layout

```text
include/
  gkinv_config.h   Configuration macros
  gkinv_types.h    C89-compatible primitive typedefs
  gkinv_fixed.h    Q16.16 integer fixed-point helpers
  gkinv.h          Main API
src/
  gkinv.c          Inventory core
  gkinv_fixed.c    Fixed-point helpers
examples/
  example_basic.c  Slot inventory
  example_grid.c   Grid inventory
  example_hooks.c  Use/effect callback
 tests/
  test_gkinv.c     Smoke tests
Makefile
```

## Build

```sh
make check
```

The default Makefile uses:

```sh
-std=c89 -pedantic -Wall -Wextra -Werror
```

## Minimal usage

```c
#include "gkinv.h"

enum {
    ITEM_POTION = 1,
    ITEM_SWORD = 2
};

static const gkinv_ItemDef items[] = {
    { ITEM_POTION, "Potion", 9u, 1u, 1u,
      GKINV_ITEM_STACKABLE | GKINV_ITEM_CONSUMABLE | GKINV_ITEM_USABLE,
      GKINV_CAT_CONSUMABLE, GKINV_FX_FROM_INT(1), 0L, 0L },

    { ITEM_SWORD, "Sword", 1u, 1u, 1u,
      GKINV_ITEM_EQUIPPABLE,
      GKINV_CAT_WEAPON, GKINV_FX_FROM_INT(3), 0L, 0L }
};

static const gkinv_ItemDB db = {
    items,
    (gkinv_u16)(sizeof(items) / sizeof(items[0]))
};

int main(void)
{
    GKINV_DECLARE_SLOTS(slots, 8u);
    gkinv_Container inv;
    gkinv_u16 index;

    gkinv_container_init(&inv, slots, 8u, GKINV_CONT_STACKS);
    gkinv_add_item(&db, &inv, ITEM_POTION, 5u, &index);
    gkinv_add_item(&db, &inv, ITEM_SWORD, 1u, &index);

    return 0;
}
```

## Core concepts

### ItemDef

An `ItemDef` is the static database definition: item id, name, max stack, grid size, flags, categories, fixed-point weight, and two project-defined integer fields.

### ItemStack

An `ItemStack` is an incoming or transferable stack with item id, quantity, flags, and four state integers.

### Slot

A `Slot` is the stored instance inside a container. It has quantity, optional grid position, flags, and four state integers. Use the state fields for ammunition loaded, durability, key state, upgrades, or project-specific metadata.

### Container

A `Container` points to a caller-owned `gkinv_Slot[]` buffer. It can behave as a simple slot inventory, grid inventory, equipment container, storage box, shop stock, crafting input, or loot table.

## Useful flags

```c
GKINV_ITEM_STACKABLE
GKINV_ITEM_CONSUMABLE
GKINV_ITEM_EQUIPPABLE
GKINV_ITEM_KEY
GKINV_ITEM_COMBINABLE
GKINV_ITEM_USABLE

GKINV_CONT_STACKS
GKINV_CONT_GRID
GKINV_CONT_EQUIPMENT
GKINV_CONT_READ_ONLY
GKINV_CONT_ALLOW_SWAP
```

## Hooks

Hooks let your engine attach gameplay without contaminating the core:

```c
typedef struct gkinv_Hooks {
    gkinv_CanAddFn can_add;
    gkinv_CanRemoveFn can_remove;
    gkinv_OnUseFn on_use;
    gkinv_EventFn on_event;
} gkinv_Hooks;
```

Common uses:

- Reject cursed items from holy containers.
- Prevent quest items from being removed.
- Heal the player when a medkit is used.
- Notify UI after an inventory change.
- Fire achievements, tutorials, quest updates, sounds, or analytics.

## Equipment slots

Use `gkinv_SlotRule` to constrain specific slots:

```c
static const gkinv_SlotRule rules[] = {
    { 0u, GKINV_CAT_WEAPON, GKINV_ITEM_EQUIPPABLE, 0UL, 1u },
    { 1u, GKINV_CAT_ARMOR,  GKINV_ITEM_EQUIPPABLE, 0UL, 1u }
};
```

Then:

```c
gkinv_container_set_slot_rules(&equipment, rules, 2u);
```

## Grid inventories

A grid container uses item `grid_w` and `grid_h` fields:

```c
gkinv_container_init(&case_inv, slots, 16u, GKINV_CONT_STACKS | GKINV_CONT_GRID);
gkinv_container_set_grid(&case_inv, 8u, 4u);
```

The core auto-places new items at the first fitting rectangle. You can move them with:

```c
gkinv_move_grid_slot(&db, &case_inv, index, x, y);
```

## Save/load

Text save/load is callback based, so the core does not own files or buffers:

```c
gkinv_save_text(&inventory, my_writer, my_user_pointer);
gkinv_load_text(&db, &inventory, loaded_text_buffer);
```

The format is intentionally simple and line-oriented.

## Important limits

`GKINV_SIM_MAX_SLOTS` controls the maximum number of slots that can be simulated on the stack for all-or-nothing operations. Default: 128.

`GKINV_GRID_MAX_CELLS` controls the max grid cell count used by temporary occupancy maps. Default: 256.

Override them from `gkinv_config.h` or compiler flags.

## Notes

- Item id `0` is reserved as empty.
- Operations are designed to be all-or-nothing when they need to mutate containers.
- The library uses linear scans on purpose: predictable, portable, and heapless.
- The UI should read slots and call API commands. Do not make the UI own the inventory state.
