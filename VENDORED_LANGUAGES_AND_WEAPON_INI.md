# Blank3D v3.7.0 — lenguajes vendorizados y armas por INI

## Sustitución de implementaciones locales

Blank3D ya no interpreta DDSL2, FPIL ni RPYL con lectores ad hoc dentro de
`monika_blank3d.c`. Las tres bibliotecas completas viven ahora en:

```text
vendor/
├── ddsl2/
├── fpil/
└── rpyl/
```

La integración común está en `src/blank3d_languages.c`:

```text
scripts/player.ddsl2
    -> compilador DDSL2
    -> bytecode DDSL2
    -> VM DDSL2
    -> acciones de movimiento/disparo del host

scripts/enemy.fpi
    -> parser/runtime FPIL
    -> condiciones y acciones registradas por Blank3D
    -> tick por enemigo

scripts/startup.rpy
    -> lexer/parser RPYL
    -> VM cuando el programa es aceptado por el validador bytecode
    -> runtime AST vendorizado como compatibilidad para sintaxis legacy válida
    -> comandos de escena registrados por Blank3D
```

Todos los buffers de integración son estáticos. DDSL2 usa dos arenas alternas
para recompilar sin destruir el programa activo; FPIL usa `load_buffered`; RPYL
recibe memoria de contexto y trabajo propiedad del runner.

`F5` vuelve a cargar los tres scripts. DDSL2 y FPIL también conservan el hot
reload por marca de tiempo ya existente.

## Registro de armas por archivos INI

El manifiesto principal es:

```text
config/weapons/weapons.ini
```

Cada entrada apunta a una definición independiente. El ID del arma sigue siendo
la identidad autoritativa del weapon manager.

```ini
[weapons]
weapon_1=pistol.ini
weapon_2=shotgun.ini
weapon_3=machine_gun.ini
```

La carga ocurre antes de enlazar los providers de cámara, raycast y presentación.
Si el manifiesto o una definición no es válida, Blank3D conserva un fallback de
perfiles compilados y lo reporta en `systems.status`.

## Esquema de una definición

### `[weapon]` — comportamiento del weapon manager

```ini
[weapon]
id=3
name=shotgun
ammo_id=2
projectile_id=3
shell_id=3
projectile_mesh_id=3
shell_mesh_id=3
fire_mode=semi
clip_size=8
ammo_per_shot=1
pellet_count=7
burst_count=1
cooldown_ms=720
reload_ms=1250
projectile_life_ms=2800
damage=8.0
speed=34.0
range=72.0
spread=7.0
projectile_radius=0.12
projectile_mesh_scale=0.36
shell_mesh_scale=0.40
recoil=0.85
```

`fire_mode` acepta `semi`, `auto`, `burst` y `hold_once`.

### `[modules]` — composición física y visual

```ini
[modules]
trigger=standard
physics=linear
trail=bullet
optic=none
sway=0
aim_query=0
emit_muzzle=1
emit_casing=1
emit_trail=1
projectile_mesh_id=3
casing_mesh_id=3
scope_preset=none
charge_time_ms=0
charge_min_speed=0
charge_max_speed=0
gravity=0
bounce=0
drag=0
mass=1
```

Valores principales:

- `trigger`: `standard`, `spinup`, `charge_release`.
- `physics`: `linear`, `gravity`, `bolt3d`.
- `trail`: `none`, `bullet`, `tracer`, `heavy`, `arc`.
- `optic`: `none`, `generic`, `sniper`.

Las cantidades físicas se convierten a Q16.16 al cargar el archivo.

### `[audio]` — familia, mecanismo y síntesis

```ini
[audio]
enabled=1
profile=shotgun
action=pump
magazine=rifle_polymer
ammo=tube
muzzle=bare
shell=shotgun_plastic
detachable_magazine=0
emits_casing=1
projectile=pellet_swarm
explosion=none
continuous_rocket=0
fire_gain_q15=30700
pressure_energy_q15=30000
ammo_motion_q15=18500
magazine_velocity_q15=23500
magazine_gain_q15=24800
reload_remove_motion_q15=27000
reload_insert_motion_q15=30500
dry_receiver_impulse=5200
action_speed=0.9
casing_velocity=130
casing_angular_velocity=155
casing_gain_q15=24800
projectile_gain_q15=23000
projectile_proximity_q15=25000
projectile_instance_limit=12
explosion_gain_q15=29200
rocket_whistle_gain_q15=10500
rocket_spin_gain_q15=5900
```

La receta sonora controla el ADN/familia, acción mecánica, tipo de alimentación,
dispositivo de boca, material de casing, voz del proyectil, explosión y afinación
de sus eventos. Los eventos siguen naciendo del weapon manager: el INI elige qué
síntesis debe responder a cada uno.

La resortera incluida mantiene `audio.enabled=0`; no recibe mecanismos de pólvora
ni casings falsos.

## Archivos incluidos

```text
config/weapons/
├── weapons.ini
├── player_weapons.ini
├── pistol.ini
├── shotgun.ini
├── machine_gun.ini
├── magnum.ini
├── gatling.ini
├── sniper.ini
├── slingshot.ini
├── grenade_launcher.ini
└── rocket_launcher.ini
```

Para crear otra arma, copia un INI, asigna un ID libre menor que la capacidad de
`GWP89_MAX_WEAPONS`, agrégalo al manifiesto y reinicia el runner.

## Pruebas nuevas

```sh
make test-languages
make test-weapon-ini
make test-audio-host
make test
```

`test-languages` ejecuta código real en DDSL2, FPIL y RPYL. `test-weapon-ini`
comprueba perfiles de escopeta, Gatling, sniper, cohete y resortera; también
verifica la conversión Q16 y los parámetros mecánico-sonoros cargados desde INI.
