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
| `bullet_circle`, `bullet_circle_count`, `bullet_circle_radius`, `bullet_circle_phase` | Origen sobre anillo; `phase` usa vueltas (`0.25` = 90°) |
| `bullet_spin`, `bullet_spin_start`, `bullet_spin_step`, `bullet_spin_direction` | Secuencia stateful de bocas/slots por actor+arma |
| `bullet_inline`, `bullet_inline_distance` | Convergencia desde un origen descentrado hacia el aim point |

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


## Infinite ammunition

Weapon profiles may opt out of clip/reserve consumption without special-casing a weapon ID:

```ini
[weapon]
infinite_ammo=1
```

Aliases: `ammo_infinite`, `infinite`. The default is `0`. When enabled, firing still obeys cooldown and trigger rules but does not consume clip/reserve and reload becomes a no-op. A provider may override the same capability with `GWP89_FLAG_INFINITE_AMMO`.

## Optional decoded muzzle image

`emit_muzzle` still controls the existing muzzle event. A raster muzzle layer is optional and is a
presentation client of the generic imgcc0 + SpritePlane89 pipeline; it does not replace procedural
muzzle presentation.

| Key | Meaning |
| --- | --- |
| `muzzle_image` / `muzzle_image_path` | Image file decoded lazily by imgcc0; empty disables the raster layer |
| `muzzle_image_width` | World-plane width, Q16 parsed from decimal text |
| `muzzle_image_height` | World-plane height |
| `muzzle_image_ms` / `muzzle_image_life_ms` | Temporary SpritePlane lifetime |
| `muzzle_image_blend` | `alpha` or `additive` |
| `muzzle_image_billboard` | `camera`, `view`, or `fixed` |
| `muzzle_image_glow` | 0/1; emits an optional second additive halo using the same image |
| `muzzle_image_glow_scale` | Q16 decimal scale for the halo plane; default `1.35` |
| `muzzle_image_glow_alpha` | 0..255 tint alpha for the halo pass; default `96` |

Example:

```ini
[modules]
emit_muzzle=1
muzzle_image=art/fx/muzzle/pistol_flash.png
muzzle_image_width=0.70
muzzle_image_height=0.70
muzzle_image_ms=70
muzzle_image_blend=additive
muzzle_image_billboard=camera
muzzle_image_glow=1
muzzle_image_glow_scale=1.35
muzzle_image_glow_alpha=96
```

## Per-weapon image muzzle flash

Conventional firearms can now choose their own image, glow and dynamic-light recipe entirely from `[modules]`:

```ini
muzzle_image=config/weapons/assets/example.jpg
muzzle_image_width=1.10
muzzle_image_height=1.10
muzzle_image_ms=58
muzzle_image_billboard=view
muzzle_image_blend=additive
muzzle_image_glow=1
muzzle_image_glow_scale=1.55
muzzle_image_glow_alpha=160
muzzle_light=1
muzzle_light_ms=72
muzzle_light_intensity=3.25
muzzle_light_radius=9.0
muzzle_light_r=255
muzzle_light_g=205
muzzle_light_b=112
```

The physical attachment point remains in `[presentation]` via `muzzle_y` and `muzzle_z`. This keeps the texture recipe independent from the weapon mechanism/socket.

See `FIREARM_MUZZLE_FLASH_CATALOG.md` for the current temporary shared-image tuning across the firearm catalog.


## Generic projectile billboard atlas recipe

A projectile visual may request an external image by logical AssetRoute89 name
or explicit path. No effect filename or atlas shape is compiled into Blank3D.

```ini
projectile_visual_image=my_effect_atlas
projectile_visual_frame_width=128
projectile_visual_frame_height=128
projectile_visual_columns=8
projectile_visual_rows=7
projectile_visual_frame_count=50
projectile_visual_frame_ms=33
projectile_visual_phase_step=7
projectile_visual_width_scale=1.55
projectile_visual_height_scale=2.10
projectile_visual_glow=1
projectile_visual_glow_scale=1.30
projectile_visual_glow_alpha=105
projectile_visual_core=1
projectile_visual_core_scale=0.72
projectile_visual_core_alpha=150
projectile_visual_light=1
projectile_visual_light_intensity=1.20
projectile_visual_light_radius=4.00
projectile_visual_light_r=255
projectile_visual_light_g=112
projectile_visual_light_b=32
```

`config/weapons/assets` is scanned as a generic AssetRoute89 image root.
Changing the animation requires only changing asset/config files, not recompiling.

## Generic projectile billboard sprite sources (five modes)

`projectile_visual_billboard=1` enables the generic projectile billboard
consumer. `projectile_sprite_mode` selects how its animation is authored:

| `projectile_sprite_mode` | Backend | Main source keys |
| --- | --- | --- |
| `static` | StaticSprite89 | `projectile_visual_image` |
| `sequence` | ImageSequencer89 | `projectile_visual_recipe` |
| `renlist` | RenList89 -> ImageSequencer89 | `projectile_visual_recipe`, `projectile_visual_animation`, `projectile_visual_clip` |
| `tilecell` | TileCell89 -> ImageSequencer89 | image + frame/cell dimensions, margin/spacing/start/step |
| `gmstrip` | GMSpritestrip89 -> SpriteAsset89 | image named like `*_stripN`; optional explicit frame count fallback |

Common controls include:

```ini
projectile_visual_loop=forward
projectile_visual_phase_step=7
projectile_visual_billboard=1
```

Loop values are `once`, `forward`, `reverse`, `pingpong`, and `hold`.
Existing glow, hot-core, additive/unlit and dynamic-light settings are shared by
all five source modes; they do not belong to a particular file layout.

Tile/grid configuration keys:

```ini
projectile_visual_frame_width=128
projectile_visual_frame_height=128
projectile_visual_columns=8
projectile_visual_rows=7
projectile_visual_frame_count=50
projectile_visual_frame_ms=33
projectile_visual_margin_x=0
projectile_visual_margin_y=0
projectile_visual_spacing_x=0
projectile_visual_spacing_y=0
projectile_visual_start_x=0
projectile_visual_start_y=0
projectile_visual_step_x=1
projectile_visual_step_y=0
```

See `PROJECTILE_SPRITE_FIVE_MODES89.md` for complete examples and authoring
formats.

## ProjectileVisual2D89 billboard authority

When `projectile_visual_billboard=1` and `projectile_sprite_mode` is not `none`, the WeaponSystem delegates 2D-in-3D projectile presentation to `ProjectileVisual2D89`.

The selected sprite mode only answers **which frame** is current. `ProjectileVisual2D89` owns the view-facing billboard geometry, visual scale/pulse, main/glow/core layers, mesh replacement and projectile light sample. Blank3D is only the asset/render provider.
