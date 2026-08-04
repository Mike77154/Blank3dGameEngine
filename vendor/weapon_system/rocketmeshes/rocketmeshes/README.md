# rocketmeshes

Low-poly static projectile + ejection/state helper mesh library for C89 game engines.

Design target:

- C89 friendly
- no malloc / realloc / free
- no heap ownership
- no float / double in runtime mesh data
- fixed-point-ish coordinates: `RMESH_Q = 8`, so 256 units = 1 mesh unit
- triangle materials are small IDs only
- purely visual dummy meshes for games/tools; not real engineering dimensions

## Main rule for realism

- `RMESH_GRENADE40_LV`: use `RMESH_SHELL_40MM_SPENT_CASE` on reload/open-breech.
- M202/Bazooka/RPG meshes: do not spawn brass shells per shot; use backblast/smoke, optional tiny debris, empty tube/clip state, or fin-state swap.

## Mesh IDs

| ID | Enum | Group | Mesh | V | Tri |
|---:|---|---|---|---:|---:|
| 0 | `RMESH_GRENADE40_LV` | projectile | 40mm grenade launcher projectile / low velocity style | 58 | 112 |
| 1 | `RMESH_M74_FLASH` | projectile | M202A1 FLASH M74 66mm rocket visual | 82 | 136 |
| 2 | `RMESH_BAZOOKA_M6A3` | projectile | 2.36 inch bazooka M6A3 inspired blunt rocket visual | 98 | 192 |
| 3 | `RMESH_BAZOOKA_M28` | projectile | 3.5 inch Super Bazooka M28/M35 inspired rocket visual | 98 | 168 |
| 4 | `RMESH_RPG7_PG7V` | projectile | RPG-7 PG-7V inspired bulb warhead visual | 114 | 200 |
| 5 | `RMESH_RPG7_PG7VR` | projectile | RPG-7 PG-7VR tandem-inspired two-lobe visual | 130 | 232 |
| 6 | `RMESH_RPG7_OG7V` | projectile | RPG-7 OG-7V inspired slim fragmentation projectile visual | 90 | 152 |
| 7 | `RMESH_SURVIVAL_RPG7_CONE` | projectile | survival-horror RPG-7 style cone warhead visual | 146 | 264 |
| 8 | `RMESH_GENERIC_ROCKET_STUB` | projectile | generic lowpoly rocket stub / fallback | 54 | 88 |
| 9 | `RMESH_SHELL_40MM_SPENT_CASE` | eject | 40mm spent case / open brass shell | 57 | 88 |
| 10 | `RMESH_DEBRIS_BAZOOKA_TAIL_CAP` | eject | optional bazooka tail-cap debris disc | 26 | 44 |
| 11 | `RMESH_DEBRIS_ROCKET_SEAL_DISC` | eject | optional broken rocket seal fragments | 12 | 8 |
| 12 | `RMESH_M202_EMPTY_ROCKET_CLIP` | eject | M202 empty four-rocket clip / cassette visual | 176 | 328 |
| 13 | `RMESH_M202_EMPTY_TUBE_CELL` | state | M202 single empty tube cell state mesh | 48 | 88 |
| 14 | `RMESH_RPG7_TAIL_FOLDED_FINS` | state | RPG-7 tail helper with folded fins | 42 | 56 |
| 15 | `RMESH_RPG7_TAIL_OPEN_FINS` | state | RPG-7 tail helper with opened fins | 42 | 56 |
| 16 | `RMESH_RE_RPG7_EMPTY_SOCKET` | state | survival RPG empty launcher socket / dark mouth | 57 | 88 |
| 17 | `RMESH_FX_BACKBLAST_RING` | fx | rocket backblast smoke ring mesh helper | 72 | 72 |

## Ejection hint table

`rm_get_eject_hint(projectile_mesh_id)` returns:

```c
typedef struct RM_EjectHint {
    rm_i16 fire_fx_mesh;       /* usually RMESH_FX_BACKBLAST_RING */
    rm_i16 fire_debris_mesh;   /* optional tiny debris, or -1 */
    rm_i16 reload_eject_mesh;  /* e.g. 40mm shell or M202 empty clip */
    rm_i16 state_empty_mesh;   /* visual state helper, not a falling shell */
    rm_u8 behavior;
} RM_EjectHint;
```

Suggested behavior mapping:

| Behavior | Meaning |
|---:|---|
| `RM_EJECT_NONE` | no automatic helper |
| `RM_EJECT_BREECH_CASE_ON_RELOAD` | spawn spent case only when opening/reloading |
| `RM_EJECT_ROCKET_SMOKE_ONLY` | rocket leaves launcher; smoke/backblast but no shell |
| `RM_EJECT_M202_TUBE_COUNTER` | mark fired tube empty; spawn empty clip only on full reload |
| `RM_EJECT_OPTIONAL_TAIL_CAP` | smoke plus small art-only debris |
| `RM_EJECT_STATE_SWAP_ONLY` | helper mesh for folded/open fin state |

## Behavior profile patch

This version adds `RM_BehaviorProfile` plus `rm_get_behavior_profile()` for realistic visual handling: cartridge case on 40mm reload, M202 empty clip/tube state, bazooka burnout/backblast behavior, and RPG-7 fin/trail phase behavior. See `docs/rocketmeshes_behavior.md` and `behavior_profiles.csv`.

## Minimal use

```c
#include "rocketmeshes.h"

const RM_Mesh *m = rm_get_mesh(RMESH_RPG7_PG7V);
const RM_EjectHint *h = rm_get_eject_hint(RMESH_RPG7_PG7V);

/* submit m->v and m->t to your own renderer */
/* h->fire_fx_mesh gives you the backblast helper, if any */
```

## Build test tools

```sh
make
./rocketmeshes_example
./rocketmeshes_dump_obj 7 obj/survival_rpg7.obj
```

## Material IDs

```c
RM_MAT_BODY = 0
RM_MAT_NOSE = 1
RM_MAT_BAND = 2
RM_MAT_FIN = 3
RM_MAT_MOTOR = 4
RM_MAT_TIP = 5
RM_MAT_DETAIL = 6
RM_MAT_BRASS = 7
RM_MAT_EMPTY = 8
RM_MAT_SMOKE = 9
```

