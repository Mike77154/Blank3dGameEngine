# API Overview

## Initialize

```c
NS_World world;
ns_init(&world);
```

For large capacities, prefer a static/global `NS_World` or engine-owned storage.

## Define types

```c
ns_define_type(&world,
               "health",
               NS_SCOPE_INSTANCE,
               NS_KIND_FIXED,
               NS_FX_FROM_INT(100),
               NS_FX_ZERO,
               NS_FX_FROM_INT(100),
               NS_OVERFLOW_CLAMP,
               NS_FLAG_SAVE | NS_FLAG_HUD);
```

## Define packs

```c
#include "numsys_packs.h"

ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT);
ns_define_pack(&world, NS_PACK_SHOOTER, NS_PACK_SHOOTER_COUNT);
```

## Attach values

```c
ns_id hp;
hp = ns_attach(&world, player_owner_id, "health");
```

## Basic actions

```c
ns_set_by_id(&world, hp, NS_FX_FROM_INT(100));
ns_add_by_id(&world, hp, NS_FX_FROM_INT(10));
ns_sub_by_id(&world, hp, NS_FX_FROM_INT(25));
ns_reset_by_id(&world, hp);
```

## Compare and percent

```c
if (ns_is_empty_by_id(&world, hp)) {
    /* dead */
}

ns_fx p;
ns_percent_by_id(&world, hp, &p);
```

## Thresholds

```c
ns_add_threshold(&world,
                 hp,
                 NS_FX_FROM_INT(25),
                 NS_EDGE_DOWN,
                 NS_TRUE,
                 1001);
```

## Events

```c
NS_Event e;
while (ns_poll_event(&world, &e) == NS_TRUE) {
    /* handle event */
}
```

## Templates

```c
ns_id enemy;
enemy = ns_define_template(&world, "enemy", 0);
ns_template_add(&world, "enemy", "health");
ns_template_add(&world, "enemy", "infection");
ns_attach_template(&world, enemy_owner_id, enemy);
```

## Modifiers

```c
ns_add_modifier_by_id(&world,
                      hp,
                      42,
                      NS_MOD_TARGET_MAX,
                      NS_MOD_FLAT,
                      NS_FX_FROM_INT(20),
                      NS_MOD_FOREVER,
                      0,
                      0);
```

## Derived values

```c
ns_add_derived(&world,
               hp_percent,
               hp,
               NS_INVALID_ID,
               NS_DERIVED_PERCENT,
               NS_FX_ONE,
               NS_FX_ZERO,
               0);
```

## Query

```c
NS_Query q;
int count;

ns_query_all(&q);
q.use_type = NS_TRUE;
q.type_id = ns_find_type(&world, "health");
q.use_cmp = NS_TRUE;
q.cmp = NS_CMP_LTE;
q.rhs = NS_FX_ZERO;

ns_query_count(&world, &q, &count);
```

## Bindings

```c
ns_bind_value(&world, hp, NS_BIND_BAR, 10, 0, 0);
```

Bindings are metadata only. Your HUD decides what `user_code` and `channel` mean.

## Snapshot export/import

```c
NS_SnapshotValue snapshot[64];
int count;

ns_export_values(&world, snapshot, 64, &count);
ns_import_values(&loaded_world, snapshot, count);
```
