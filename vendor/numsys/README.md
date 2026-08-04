# NUMSYS C89 v0.3.0

NUMSYS is a tiny fixed-point numeric-system library for games, simulations and engines.
It treats values like `health`, `ammo`, `stamina`, `infection`, `score`, `oxygen`, `armor`, `sanity`, `weapon.heat` and custom meters as configurable numeric systems instead of loose variables.

## Contract

```txt
C89
no heap allocation inside the library
no stdio dependency inside the library
no float/double types in the library API or implementation
fixed-point only: ns_fx, where NS_FX_ONE == 1024
single caller-owned NS_World struct
static-capacity arrays only
```

## What's new in v0.3.0

```txt
Core retained from v0.1
├─ define numeric types
├─ attach values to owners
├─ get/set/add/sub/reset
├─ min/max/current/previous
├─ clamp/wrap/bounce overflow
├─ manual shield spill
├─ thresholds
├─ event queue
└─ tick regen/drain

New integrated systems
├─ optional preset packs
│  ├─ arcade
│  ├─ survival
│  ├─ shooter
│  └─ rpg
│
├─ templates/families
│  ├─ define template enemy
│  ├─ add health/infection/armor to it
│  └─ attach template to many owners
│
├─ attribute-style base/current model
│  ├─ base_value
│  ├─ final value
│  ├─ base min/max
│  ├─ final min/max
│  └─ dirty/changed flags
│
├─ fixed-point modifiers
│  ├─ flat
│  ├─ percent additive
│  ├─ percent multiplicative
│  ├─ target value/min/max/regen/drain
│  └─ timed expiration by ticks
│
├─ derived values
│  ├─ copy
│  ├─ sum/sub/product/ratio
│  ├─ min/max
│  ├─ percent
│  └─ lerp
│
├─ queries
│  ├─ pick values by owner/type/comparison/tag/flags
│  ├─ count matching values
│  ├─ add/sub/set all matches
│  └─ event-sheet-ish workflow
│
├─ bindings
│  ├─ bar
│  ├─ counter
│  ├─ ECG
│  ├─ icon
│  ├─ debug
│  └─ custom user code/channel
│
└─ snapshots
   ├─ export values to caller-owned memory
   ├─ import values from caller-owned memory
   └─ callback-based save/load hooks without stdio
```

## Folder layout

```txt
numsys_c89_v0_3_0/
├─ include/
│  ├─ numsys.h
│  └─ numsys_packs.h
├─ src/
│  └─ numsys.c
├─ examples/
│  ├─ example_basic.c
│  ├─ example_construct_fusion_style.c
│  ├─ example_modifiers.c
│  └─ example_snapshot.c
├─ tests/
│  └─ test_numsys.c
├─ docs/
│  ├─ DESIGN.md
│  └─ API_OVERVIEW.md
├─ Makefile
└─ README.md
```

## Build

```sh
make
make check
```

The default Makefile uses:

```sh
-std=c89 -pedantic -Wall -Wextra -Werror
```

## Basic use

```c
#include "numsys.h"
#include "numsys_packs.h"

static NS_World world;

int main(void)
{
    ns_id hp;
    ns_id ammo;

    ns_init(&world);
    ns_define_pack(&world, NS_PACK_SURVIVAL, NS_PACK_SURVIVAL_COUNT);
    ns_define_pack(&world, NS_PACK_SHOOTER, NS_PACK_SHOOTER_COUNT);

    hp = ns_attach(&world, 1, "health");
    ammo = ns_attach(&world, 2, "ammo.magazine");

    ns_sub_by_id(&world, hp, NS_FX_FROM_INT(25));
    ns_sub_by_id(&world, ammo, NS_FX_FROM_INT(1));

    return 0;
}
```

## Templates / families

```c
ns_id enemy_template;

enemy_template = ns_define_template(&world, "enemy", 0);
ns_template_add(&world, "enemy", "health");
ns_template_add(&world, "enemy", "infection");
ns_template_add(&world, "enemy", "armor");

ns_attach_template(&world, 101, enemy_template);
ns_attach_template(&world, 102, enemy_template);
ns_attach_template(&world, 103, enemy_template);
```

This gives each owner its own independent numeric slots, similar to alterable/instance values.

## Modifiers

```c
/* +50 max stamina, permanent */
ns_add_modifier_by_id(&world, stamina,
                      100,
                      NS_MOD_TARGET_MAX,
                      NS_MOD_FLAT,
                      NS_FX_FROM_INT(50),
                      NS_MOD_FOREVER,
                      0,
                      0);

/* +50% current value for 2 ticks */
ns_add_modifier_by_id(&world, stamina,
                      200,
                      NS_MOD_TARGET_VALUE,
                      NS_MOD_PERCENT_ADD,
                      NS_FX_FROM_PERCENT(50),
                      2,
                      0,
                      0);
```

## Queries

```c
NS_Query q;
int affected;

ns_query_all(&q);
q.use_type = NS_TRUE;
q.type_id = ns_find_type(&world, "health");
q.use_cmp = NS_TRUE;
q.cmp = NS_CMP_LTE;
q.rhs = NS_FX_ZERO;

ns_query_count(&world, &q, &affected);
```

## Snapshots

```c
NS_SnapshotValue snapshot[64];
int count;

ns_export_values(&world, snapshot, 64, &count);
ns_import_values(&other_world, snapshot, count);
```

NUMSYS does not own files or streams. Your engine decides how to store the exported memory.
