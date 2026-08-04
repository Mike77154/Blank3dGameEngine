# Referencia rápida de `config/weapons/*.ini`

## `[weapon]`

| Clave | Uso |
|---|---|
| `id`, `name` | Identidad del arma |
| `gun_id`, `ammo_id`, `projectile_id`, `shell_id` | IDs del weapon stack |
| `muzzle_id`, `casing_id`, `trail_id` | IDs de eventos/presentación |
| `projectile_mesh_id`, `shell_mesh_id` | Catálogos visuales |
| `fire_mode` | `semi`, `auto`, `burst`, `hold_once` |
| `clip_size`, `ammo_per_shot`, `pellet_count`, `burst_count` | Alimentación y patrón |
| `fire_rate_bps`, `fire_rate_rpm`, `cooldown_ms` | Cadencia; use una sola forma por perfil |
| `reload_ms`, `projectile_life_ms` | Recarga y vida del proyectil |
| `active_reload_*` | Ventana, premio y castigo de recarga activa |
| `damage`, `projectile_speed`, `range`, `spread` | Balística base (`speed` queda como alias legado) |
| `projectile_radius`, `projectile_mesh_scale`, `shell_mesh_scale` | Colisión/dibujo |
| `recoil` | Retroceso enviado a CamaraNaku |

## `[modules]`

| Clave | Valores principales |
|---|---|
| `trigger` | `standard`, `spinup`, `charge_release` |
| `physics` | `linear`, `gravity`, `bolt3d` |
| `trail` | `none`, `bullet`, `tracer`, `heavy`, `arc` |
| `optic` | `none`, `generic`, `sniper` |
| `sway`, `aim_query` | 0/1 |
| `emit_muzzle`, `emit_casing`, `emit_trail` | 0/1 |
| `scope_preset` | Nombre registrado en gscopepresets89 |
| `charge_time_ms`, `charge_min_speed`, `charge_max_speed` | Armas de carga |
| `gravity`, `bounce`, `drag`, `mass` | Física Q16.16 |

## `[audio]`

| Clave | Uso |
|---|---|
| `profile` | ADN sonoro: pistol, smg, shotgun, magnum, sniper, launcher, heavy |
| `action` | pistol, machine, pump, revolver, rifle |
| `magazine`, `ammo` | Alimentación y movimiento de munición |
| `muzzle`, `shell` | Boca y material del casing |
| `projectile` | none, near_miss, supersonic, pellet_swarm, tracer |
| `explosion` | none, grenade, rocket |
| `continuous_rocket` | Mantiene whistle/spin durante la vida del proyectil |
| `*_gain_q15`, `*_motion_q15` | Ganancia y energía fixed-point |
| `action_speed` | Velocidad Q16.16 del mecanismo |
| `casing_velocity`, `casing_angular_velocity` | Excitación del casing |
| `projectile_instance_limit` | Polifonía de voces de proyectil |

Los nombres desconocidos usan el valor seguro por defecto de su categoría. Un
archivo sin `id` o `name` cancela la carga del manifiesto y activa el fallback.

## Cadencia frente a velocidad

```ini
fire_mode=auto
fire_rate_bps=13.333
projectile_speed=46.0
```

- `fire_rate_bps`: disparos por segundo.
- `fire_rate_rpm`: disparos por minuto.
- `cooldown_ms`: milisegundos exactos entre disparos.
- `projectile_speed`: unidades recorridas por segundo por la bala.
- `[audio] action_speed`: velocidad del mecanismo sonoro; no cambia cadencia.

Si aparecen varias claves de cadencia en el mismo bloque, prevalece la última.
