# Porting Notes

## Memory model

GKINV never allocates. Every container receives a caller-owned slot array:

```c
GKINV_DECLARE_SLOTS(player_slots, 8u);
gkinv_container_init(&player_inv, player_slots, 8u, GKINV_CONT_STACKS);
```

For ROM/static projects, define item databases as `static const gkinv_ItemDef[]`.

## C89 style

The code avoids:

- C99 declarations in `for` statements.
- C++-style comments.
- compound literals.
- designated initializers.
- variable length arrays.
- `stdint.h` and `stdbool.h`.

## Tuning stack usage

All-or-nothing operations clone containers into local temporary arrays. Defaults:

```c
#define GKINV_SIM_MAX_SLOTS 128u
#define GKINV_GRID_MAX_CELLS 256u
```

For tiny platforms, reduce them. For larger inventories, increase them.

## Disabling modules

From `gkinv_config.h`:

```c
#define GKINV_ENABLE_TEXT_IO 0
#define GKINV_ENABLE_RECIPES 0
#define GKINV_ENABLE_GRID 0
#define GKINV_ENABLE_HOOKS 0
```

## Suggested integration layers

```text
engine/
  inventory_core/      GKINV files
  item_defs/           your static item database
  save_system/         calls gkinv_save_text/load_text
  ui_inventory/        reads slots and calls commands
  gameplay_hooks/      heal, equip, unlock, quest effects
  script_bindings/     DDSL2/FPI/RPY wrappers
```
