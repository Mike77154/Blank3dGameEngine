# Provider integration matrix

| Service | Main operations | Provider may affect | Fallback when not handled |
|---|---|---|---|
| `actor_state` | `UPDATE` | effective input, delta time, cooldown/reload/latch advancement | internal actor timers and trigger release |
| `math3d` | `MATH_ADD`, `MATH_SCALE`, `MATH_NORMALIZE` | vector arithmetic used by aiming and endpoints | internal fixed-point vector math |
| `transform` | `GET_ACTOR_TRANSFORM` | actor position, basis, and scale | values copied from fire input |
| `numeric` | `QUERY_INT`, `QUERY_FX` | fire mode, ammo cost, pellets, burst, timings, damage, speed, range, spread, scales, recoil | weapon profile value |
| `flags` | `QUERY_FLAG`, `FIRE_VALIDATE`, `FIRE_ACCEPTED` | permissions, subsystem emission, validation, cancellation | internal weapon policy |
| `camera` | `GET_CAMERA` | camera origin/basis/view style | fire-input camera/socket values |
| `zoom` | `QUERY_FX`, `EMIT_EVENT` | effective zoom and visual reactions | input zoom or fixed one |
| `sockets` | `GET_SOCKET` | projectile, muzzle, casing, and aim transforms | actor/input transform |
| `inventory` | ammo/clip/equip operations and events | reserve ammo, magazines, consumption, equipment ownership | legacy ammo hooks, then internal bank/state |
| `raycast` | `RAYCAST` | hit actor/material/point/normal/distance | fixed-point endpoint with no hit |
| `pose` | `RESOLVE_POSE` | complete projectile/muzzle/casing pose | legacy pose hook, then providerized component pipeline |
| `spread` | `APPLY_SPREAD` | direction per pellet | deterministic internal spread |
| `projectile_life` | `PROJECTILE_LIFE` | lifetime, range, travel distance | numeric/profile values |
| `projectile` | `EMIT_EVENT` | projectile or hitscan spawn | legacy projectile hook, then queue only |
| `health` | `DAMAGE_REQUEST` | damage application for valid ray hits | event remains informational |
| `muzzle` | `EMIT_EVENT` | muzzle flash/effect spawn | legacy muzzle hook, then queue only |
| `casing` | `EMIT_EVENT` | casing spawn/ejection | legacy casing hook, then queue only |
| `trail` | `EMIT_EVENT` | tracer/trail spawn | legacy trail hook, then queue only |
| `mesh` | `EMIT_EVENT` | projectile/shell mesh assignment | legacy mesh hook, then queue only |
| `recoil` | `EMIT_EVENT` | camera/weapon recoil response | recoil value remains in event |
| `reload` | begin/tick/complete plus events | normal reload lifecycle and transfer | internal timed reload with inventory bridge |
| `active_reload` | `ACTIVE_RELOAD_PRESS` plus events | success window, bonus, penalty, or entire minigame | optional internal active reload |
| `hud` | `EMIT_EVENT` | ammo, weapon, reload, fire, and status UI | event queue |
| `crosshair` | `EMIT_EVENT` | crosshair feedback and state | event queue |
| `scope` | `EMIT_EVENT` | scope/reticle feedback | event queue |
| `draw` | `EMIT_EVENT` | generic weapon visualization/debug draw | legacy visual hook or event queue |
| `event_bus` | `EMIT_EVENT` | replication, telemetry, scripting, orchestration | static event queue |
| `io` | `READ_TEXT_FILE` | path/resource-to-text loading | caller uses `gwp89_load_ini_text()` |

All rows use the same priority-ordered PRE/POST ABI. `HANDLED` replaces the listed fallback, `MODIFIED` stacks changes, `CANCEL` aborts the stage, and `STOP` prevents lower-priority providers from running.
