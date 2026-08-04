# gklek89 v1.0

A tiny procedural dry mechanical **klek/click** synthesizer for weapon and machinery Foley.

## Constraints

- strict C89;
- fixed 44.1 kHz mono core;
- integer/fixed-point only;
- no `malloc`, `realloc`, `free`, heap, `float`, `double`, or `math.h`;
- caller-owned context;
- deterministic seeded variation;
- CC0.

## Cheap synthesis model

`impact impulse + short filtered noise burst + two tiny feedback comb resonators`

The impulse creates the hard contact, the decaying noise creates the dry scrape/particle edge, and the two combs create a compact metal receiver cavity without convolution or trigonometry.

## Presets

- `GKL89_PRESET_DRY_CLICK`
- `GKL89_PRESET_STEEL_KLEK`
- `GKL89_PRESET_CARRIER_KLEK`
- `GKL89_PRESET_REAR_STOP`

`carrier_klek` is the intended midpoint layer for the shotgun pump assembly.

## Build

```sh
make test
```
