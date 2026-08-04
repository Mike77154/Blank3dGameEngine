# weapon_synth_sound_engine89 v1.9.0

Motor procedural de sonido de armamento en **C89 estricto**, aritmética entera/fixed-point y memoria propiedad del caller. El release consolida todas las librerías vendorizadas en un solo grafo de compilación y añade una fachada de eventos neutral respecto del game engine.


## Novedad v1.9.0 — Rocket propulsion + Gatling motor stack

- Integra `grocketspin89 v1.0` como fondo rotativo de la trayectoria del cohete.
- Refuerza la propulsión inicial mediante `gpaah89`, pulso de presión,
  expansión de boca y una llamarada corta de `gfire89`.
- Añade durante el vuelo una capa discreta de `gfire89` tipo torch, por debajo
  de `grocketwhistle89 v1.2` y `grocketspin89`.
- Conserva el impacto como evento independiente con `wsound_rocketblast89`,
  rumble, crackle y debris.
- Integra simultáneamente `ggunmach89`, `GGunTuberotator89` y
  `ggatlingwhistle89` como colchón motorizado de Gatling.
- Añade una secuencia Gatling completa: caja/cinta, spin-up, alimentación,
  ráfaga de 80 tiros a 3000 rpm, receiver, casquillos y spin-down.
- Añade siete eventos append-only para controlar spin de cohete y Gatling
  desde cualquier game engine.
- Añade prueba de integración, showcase dedicado, sanitizadores y validación
  determinista sin clipping.

Consulta [`ROCKET_SPIN_GATLING_INTEGRATION_V1_9.md`](ROCKET_SPIN_GATLING_INTEGRATION_V1_9.md).

## Novedad v1.8.2 — Secuencias mecánicas físicas

- Separa el **ciclo automático por disparo** de la **recarga manual**.
- Pistola: corredera corta, extracción y casing por tiro; `wmagazine89` sólo durante el cambio de cargador.
- Magnum: martillo/cilindro por tiro; apertura, extractor y seis vainas durante la recarga.
- Escopeta: pump completo y shell expulsado después de cada tiro.
- Ametralladora belt-fed: bolt/feed pawls, avance de cinta, casing y link por tiro; apertura/cambio de cinta sólo al recargar.
- Sniper bolt-action: unlock, pull, eject, push y lock después de cada tiro; cargador únicamente en la recarga.
- Lanzacohetes: carga, ignición, silbido discreto, impacto y debris; el silbido fue reducido 10.81 dB RMS frente a v1.8.1.
- Añade resonancia de receiver a los bancos mecánicos y dos bancos especializados: corredera automática de pistola y ciclo interno belt-fed.

Consulta [`PHYSICAL_MECHANICS_SEQUENCE_V1_8_2.md`](PHYSICAL_MECHANICS_SEQUENCE_V1_8_2.md).

## Novedad v1.8.1 — Cargadores y trayectoria de cohete

- Integra `wmagazine89` en la biblioteca agregada y en el ABI de eventos.
- Añade acciones de cargador: extracción, inserción, golpe de asiento, tug-check y rattle.
- Actualiza las recargas de pistola y sniper con presets físicos de cargador; la ametralladora conserva `wsoundbeltfeed89` porque su demo es alimentado por cinta.
- Integra `grocketwhistle89` con Doppler, EQ de seis bandas, chorus y reverb opcional.
- Separa la secuencia de cohete en ignición, silbido de trayectoria, liberación e impacto.
- Añade pools caller-owned de cuatro voces de cargador y cuatro voces de silbido, sin heap.
- Añade cinco eventos append-only y una prueba de integración determinista.

Consulta [`MAGAZINE_ROCKET_WHISTLE_INTEGRATION_V1_8_1.md`](MAGAZINE_ROCKET_WHISTLE_INTEGRATION_V1_8_1.md).

## Novedad v1.8.0 — Cargadores completos y recargas

- Añade previews de un tiro aislado seguido de la capacidad completa para pistola, Magnum, escopeta, ametralladora, sniper y lanzacohetes.
- Conserva disparo, mecanismos, casquillos/shells y recarga en una sola secuencia por arma.
- Corrige la conversión de milisegundos a frames para líneas de tiempo de más de 97 segundos.


## Novedad v1.7.0 — Jerarquía física y secuencias completas

- Añadió perfiles de mezcla `REALISTIC`, `HYBRID` y `CINEMATIC` al Voice Handler.
- `REPORT` y `EXPLOSION` dominan; mecanismo, Foley, casquillos e impactos permanecen audibles sin superar el evento principal.
- El evento `WEAPON_MODE` sincroniza DNA, acústica y jerarquía del mixer.
- Rebalanceó reportes, shotgun master, granada y rocket blast.
- Añadió secuencias completas de pistola, Magnum, escopeta, ametralladora, sniper, granada, lanzacohetes y lanzagranadas.
- Añadió un showcase continuo, ocho WAV individuales y validación automática de la diferencia entre evento principal y mecanismo.

Consulta [`WEAPON_HIERARCHY_AND_SEQUENCES_V1_7.md`](WEAPON_HIERARCHY_AND_SEQUENCES_V1_7.md).

## Novedad v1.6.2 — Shotgun Pump Master

- Añadió `gshotguneq89`, un máster fixed-point de seis bandas exclusivo del bus mecánico de escopeta.
- Añadió perfiles `BALANCED`, `REMINGTON_870`, `MOSSBERG_500_590`, `BENELLI_NOVA` y `WINCHESTER_SXP`.
- Masteriza conjuntamente `gpump89`, `chuecka89`, `shotpumpkin89`, `gklek89` y el tik/tek final.
- `gklek89` permanece como articulación real del carrier/lock dentro del ciclo.
- Añadió proveedor opcional `tek` para conectar una librería externa sin dependencia dura; el Foley tik anterior permanece como fallback.
- Añadió comparación A/B, comparación de cuatro modelos y secuencia completa masterizada.

Consulta [`SHOTGUN_MASTER_PHYSICS_V1_6_2.md`](SHOTGUN_MASTER_PHYSICS_V1_6_2.md).

## Novedad v1.6 — Fase 3

- Weapon DNA físico con diez perfiles editables.
- Variación correlacionada y determinista por disparo.
- Aplicación coherente a report, body, gas, crack, thump, tail y receiver.
- Modos globales `REALISTIC`, `HYBRID` y `CINEMATIC`.
- `wsoundmetrics89`: peak, RMS entero, crest, low/mid/high, ataque, caída,
  zero-crossing y clipping.
- Targets editables, validación por flags y score Q15.
- Eventos `WEAPON_PROFILE`, `WEAPON_MODE`, `WEAPON_FIRE`,
  `METRICS_ENABLE` y `METRICS_RESET`.

Consulta [`PHASE3_DNA_METRICS_V1_6.md`](PHASE3_DNA_METRICS_V1_6.md).

## Novedad v1.5 — Fase 1 + Fase 2 acústica

La biblioteca completa ahora incluye `wsoundacoustic89`, un stack caller-owned que añade:

- pulso de presión bipolar para disparos, impactos, granadas y cohetes;
- propagación low/mid/high con distancia, aire, directividad, oclusión, material y portal;
- armónico grave de traducción para altavoces pequeños;
- combat bus multibanda y limitador;
- ocho reflexiones tempranas;
- cuatro líneas de reverberación tardía;
- materiales con absorción/transmisión por bandas;
- espacios interiores/exteriores, ground bounce y portales;
- perfiles `REALISTIC`, `HYBRID` y `CINEMATIC`.

La guía completa, memoria, limitaciones y ejemplo de eventos están en [`PHASE1_PHASE2_ACOUSTICS_V1_5.md`](PHASE1_PHASE2_ACOUSTICS_V1_5.md).

## Estado

- Sin `malloc`, `realloc`, `free`, `float` ni `double` en el código C/H activo.
- Sin dependencia obligatoria de SDL, OpenAL, FMOD, Wwise ni de un motor específico.
- Audio PCM16 estéreo; sample rate validado entre 8 kHz y 48 kHz.
- Semillas deterministas; `seed == 0` solicita una semilla determinista generada por el contexto.
- Pools, delays y voces suministrados por el caller.
- Build verificado con `-std=c89 -pedantic -Wall -Wextra -Werror`.
- Verificado con AddressSanitizer, UndefinedBehaviorSanitizer e i386 `-m32`.

## Construcción

```sh
make all
make test
make render
```

`make all` produce:

```text
build/libweapon_synth_sound_engine89.a  # biblioteca completa, autocontenida
build/libgsynthreport89.a               # reportes/disparos
build/libgsynthsoundengine89.a          # casquillos y fuego
build/libgsynthordnance89.a             # bala, granada y cohete
build/libgsynthsoundexpansion89.a       # aero, fricción, belt, portal, etc.
build/libgsynthworld89.a                # projectile/impact/ricochet/world
build/libweapon_synth_event89.a         # fachada delgada; requiere módulos elegidos
```

Para la integración sencilla enlaza únicamente `libweapon_synth_sound_engine89.a` e incluye:

```c
#include "weapon_synth_sound_engine89.h"
```

## Arquitectura

```text
EVENTOS DEL GAME ENGINE
        │
        ▼
wsse89_event_sink / wsse89_dispatch
        │
        ├── reportes: gpaah + body + muzzle + crack + tail + thump
        ├── base: casquillos + fuego/lanzallamas
        ├── ordnance: bullet-air + grenade-blast + rocket-blast
        ├── expansion: muzzle device, aero, friction, particles, ammo,
        │              belt feed, cargadores, outdoor, portal, doppler,
        │              rocket whistle, spatial, thermal, listener y mask
        └── world: projectile, impact, ricochet, DNA, receiver,
                   propagation, room, action, combat bus, acoustic phase 1/2,
                   Weapon DNA físico y métricas Phase 3
                          │
                          ▼
                 gvoice89 / gweaponvoice89
                          │
                          ▼
                 PCM16 ESTÉREO DEL HOST
```

Las librerías mecánicas especializadas (`chuecka89`, `gpump89`, `gweaponfoley89`, `gklek89`, `gshotgunsequence89`, `wmagazine89`) también se compilan dentro de la biblioteca completa y quedan públicamente expuestas por el encabezado paraguas. Así un motor puede usar el bus unificado para eventos comunes y las APIs de bajo nivel para secuencias mecánicas específicas sin enlazar otra copia.

## Flujo de integración

1. Reserva pools estáticos o arenas del host y rellena `wsse89_storage`.
2. Inicializa `wsse89_context` con `wsse89_init`.
3. Desde gameplay traduce disparos, impactos, acciones y cambios espaciales a `wsse89_event`.
4. Desde el callback de audio llama `wsse89_render_stereo` por bloques o `wsse89_process_stereo_sample` por muestra.
5. Para eventos con vida propia conserva el `gv89_handle` devuelto y úsalo con `STOP_HANDLE` o `SET_HANDLE_SPATIAL`.

Ejemplo mínimo de un disparo:

```c
wsse89_event event;
gv89_handle handle;

event.type = WSSE89_EVENT_REPORT;
event.seed = 0U;
gssr89_defaults(&event.data.report, GPAAH89_PRESET_PISTOL);
event.data.report.pan_q15 = 0;

(void)wsse89_dispatch(&audio, &event, &handle);
```

El archivo `examples/game_engine_event_bridge.c` muestra un adaptador que guarda `wsse89_event_sink_fn` y convierte callbacks de gameplay en eventos C89.

## Superficie de eventos

La fachada expone 45 eventos despachables más `NONE` (46 valores públicos antes de `EVENT_COUNT`), entre ellos:

- reportes, casquillos, fuego, paso de bala, granadas y cohetes;
- el payload de cohete puede usar presets o transportar `wsrb89_params` completos, incluyendo rumble, crackle, SVF y LFO;
- proyectiles, impactos y ricochets;
- muzzle devices, belt feed, fricción, aero, partículas, munición y térmicos;
- listener exposure, máscara, azimut, portales y mezcla exterior;
- acciones mecánicas, receiver excitation, stop y actualización espacial por handle.

Consulta `EVENT_ABI_GUIDE_V1_6.md` para el mapeo completo de payloads.

## Acceso directo

El contexto conserva subcontextos públicos para integración avanzada:

```c
ctx.reports
ctx.base
ctx.ordnance
ctx.expansion
ctx.world
ctx.handler
```

Esto permite configurar DNA, habitaciones, propagación, combat bus, secuenciadores y módulos DSP sin atravesar la unión de eventos cuando el host necesita control fino.

## Preview

```sh
make render
```

El preview consolidado es:

```text
audio/00_v1_6_0_unified_event_showcase.wav
```

Previews específicos del cohete v1.4:

```text
audio/12_rocketblast_rumble_crackle_layers.wav
audio/13_rocketblast_ab_5noise_vs_7noise.wav
```

Secuencia launcher integrada en los demos:

```text
audio/04_full_firefight_plus_gtinkle_gfire_rocket.wav
audio/06_rocket_launcher_launch_to_impact.wav
```

En ambos, `GPAAH89_PRESET_ROCKET_LAUNCHER` representa la ignición/salida
del tubo y `WSSE89_EVENT_ROCKET_BLAST` representa el impacto remoto. Son dos
eventos separados para que un game engine pueda dispararlos desde `weapon_fire`
y `projectile_collision`, respectivamente.

Contiene pistola, escopeta, ráfaga, proyectil, impacto metálico, ricochet, granada, cohete, lanzallamas, aero, partículas, munición y sniper; se genera usando la misma API pública de eventos que utilizaría un game engine.


Previews v1.8.1:

```text
audio/06_rocket_launcher_launch_to_impact.wav        # ignición -> whistle -> impacto
audio/16_pistol_one_plus_20_full_reload.wav          # wmagazine pistol
audio/20_sniper_one_plus_4_full_reload.wav            # wmagazine sniper box
audio/21_rocket_one_plus_4_full_reload.wav            # cinco trayectorias completas
audio/22_all_one_shot_then_full_capacity_sequences.wav
```

## Validación incluida

- `BUILD_REPORT_V1_6.txt`
- `SANITIZER_REPORT_V1_6.txt`
- `I386_BUILD_V1_6.txt`
- `PROTOCOL_SCAN_V1_6.txt`
- `DETERMINISM_REPORT_V1_6.txt`
- `WAV_VALIDATION_V1_6.json`
- `RELEASE_MANIFEST_V1_6.txt`
- `SHA256SUMS_V1_6.txt`

Preview Phase 3:

```text
audio/16_phase3_realistic_hybrid_cinematic.wav
PHASE3_METRICS_V1_6.json
```

Licencia: CC0, conforme al archivo `LICENSE`.


## v1.6.1 triple-layer pump action

The complete shotgun pump in `gshotgunsequence89` now layers three independent
procedural sources:

- `gpump89`: low/mid structural mechanism stages and shell ejection;
- `chuecka89`: bright articulated metal transients;
- `shotpumpkin89`: continuous action-bar friction, rail chatter and endpoint texture.

`shotpumpkin89` is vendored and compiled into the aggregate library. Its pull,
reversal gap and forward-pump durations are stretched to the `gpump89` rear-stop,
forward-friction and lock anchors. The new layer is controlled with
`gss89_mix.pump_shotpumpkin_q15`, `pump_shotpumpkin_delay_ms`, and
`gss89_set_shotpumpkin_preset()`. Setting its gain to zero restores the previous
two-synth pump mix.

## Preview de Shotgun Pump Master v1.6.2

```text
audio/wsse89_v1_6_2_shotgun_master_ab.wav
audio/wsse89_v1_6_2_shotgun_model_pumps.wav
audio/wsse89_v1_6_2_full_shotgun_sequence_mastered.wav
```

El A/B reproduce el mismo pump y semilla primero sin el máster y después con
`gshotguneq89`. La comparación de modelos presenta, en orden, Remington 870,
Mossberg 500/590, Benelli Nova y Winchester SXP. Los perfiles son tunings
procedurales informados por mecanismo y escucha cualitativa; no contienen ni
redistribuyen grabaciones externas.

## v1.8.0 full-capacity showcases

Run:

```sh
make build/render_complete_weapon_sequences
./build/render_complete_weapon_sequences
```

New outputs `audio/16` through `audio/22` demonstrate one isolated shot followed by a full magazine/capacity and complete reload handling for pistol, Magnum, shotgun, machine gun, sniper rifle, and rocket launcher. See `FULL_CAPACITY_SEQUENCES_V1_8.md` and `FULL_CAPACITY_WAV_VALIDATION_V1_8.json`.



## v1.9.0 rocket propulsion and Gatling stack

- `grocketspin89` is a native trajectory provider alongside `grocketwhistle89`.
- A low-level `gfire89` voice may be layered during rocket flight for restrained sustainer-plume texture.
- `ggunmach89`, `GGunTuberotator89`, and `ggatlingwhistle89` are vendored and exposed as one coordinated Gatling background stack.
- New events separate spool-up, fire-loop load, spool-down, rocket spin start/motion/stop.
- `render_rocket_spin_gatling_showcase` renders a rocket launch/flight/impact and a complete M134-like Gatling sequence.
