# wsound weapon-world suite 89 v1.0

Nine independent C89 fixed-point modules for procedural weapon sound in a real-time game world.

## Modules

- `wsoundprop89`: delay, distance, directivity, occlusion
- `wsoundprojectile89`: ballistic N-wave and flybys
- `wsoundroom89`: shared early reflections and tail
- `wsoundaction89`: mechanical event sequencing
- `wsounddna89`: correlated shot variation
- `wsoundimpact89`: material impacts
- `wsoundricochet89`: ricochet chirps and debris
- `wsoundreceiver89`: structural/modal weapon coloration
- `wsoundcombatbus89`: dense-combat clarity and limiting

All cores are C89, fixed-point/integer only, deterministic, and contain no allocation calls. `wsoundprop89` and `wsoundroom89` receive their delay memory from the host.

Build every module with `./build_all.sh`.
