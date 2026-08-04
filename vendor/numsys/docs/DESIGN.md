# NUMSYS Design Notes

NUMSYS is not a variable table. It is a fixed-capacity numeric system registry.

## Philosophy

```txt
A number is not just storage.
A number can have identity, owner, limits, modifiers, events, bindings and save state.
```

`health`, `ammo`, `infection`, `sanity`, `oxygen`, `score`, `recoil`, `noise`, `weapon.heat` and `generator.voltage` are all the same kind of creature: a numeric slot with rules.

## Main layers

```txt
NS_World
├─ Type definitions
│  └─ named blueprints: health, ammo.magazine, infection...
│
├─ Values
│  └─ owner + type = actual numeric slot
│
├─ Templates
│  └─ attach reusable sets of numeric slots to owners
│
├─ Events
│  └─ on changed, empty, full, threshold, modifier expired...
│
├─ Modifiers
│  └─ fixed-point buffs/debuffs over value/min/max/regen/drain
│
├─ Derived values
│  └─ calculated outputs such as health.percent
│
├─ Queries
│  └─ event-sheet style selection and batch actions
│
├─ Bindings
│  └─ metadata for HUD/debug/UI without depending on a renderer
│
└─ Snapshots
   └─ caller-owned save/load data
```

## Fixed-point

`ns_fx` is a signed integer type. `NS_FX_ONE` is `1024`, so one unit is represented as `1024`.

```c
NS_FX_FROM_INT(100)      /* 100.0 */
NS_FX_FROM_RATIO(1, 4)   /* 0.25  */
NS_FX_FROM_PERCENT(50)   /* 0.50  */
```

## Base/current model

Each value stores both base and final values:

```txt
base_value
└─ modifiers
   └─ final value
```

Bounds and rates also have base/final pairs:

```txt
base_min_value       -> min_value
base_max_value       -> max_value
base_regen_per_tick  -> regen_per_tick
base_drain_per_tick  -> drain_per_tick
```

This keeps runtime values simple while still supporting attribute-like modifiers.

## Modifier order

For one target, NUMSYS applies modifiers in this order:

```txt
1. flat
2. percent additive, combined
3. percent multiplicative, applied sequentially
```

This mirrors common stat-system behavior but avoids dynamic containers.

## Templates

Templates are reusable lists of type IDs. They are inspired by families and object presets.

```txt
template enemy
├─ health
├─ infection
└─ armor
```

Attaching the template to many owners gives each owner independent numeric slots.

## Events

Events are pushed into a fixed-size queue. You can poll them or dispatch them through a callback.

```txt
NS_EVENT_CHANGED
NS_EVENT_INCREASED
NS_EVENT_DECREASED
NS_EVENT_EMPTY
NS_EVENT_FULL
NS_EVENT_THRESHOLD
NS_EVENT_RESET
NS_EVENT_MODIFIER_ADDED
NS_EVENT_MODIFIER_REMOVED
NS_EVENT_MODIFIER_EXPIRED
NS_EVENT_DERIVED_UPDATED
```

## Persistence

NUMSYS does not use file APIs. It exports `NS_SnapshotValue` records into caller-owned memory, and also provides callback hooks for engines that want direct binary writing through their own VFS.
