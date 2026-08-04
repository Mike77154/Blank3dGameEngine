# Blank3D v3.25.4 — Automatic Fire Frame Lock


## v3.25.4 cámara inmutable durante fuego automático

La cámara, la cruz, el arma montada, el muzzle, los trazadores y los
proyectiles consumen ahora una sola base inmutable por frame. Los eventos
`FIRE_ACCEPTED` ya no aplican recoil en medio del frame: acumulan la solicitud
y la confirman antes de capturar la cámara del frame siguiente. Esto elimina
el desfasamiento que aparecía al caminar, rotar la cámara y sostener fuego con
la ametralladora o la Gatling.

La regresión `make test-automatic-fire-frame-lock` mantiene movimiento, giro y
fuego durante 900 frames y más de dos rotaciones completas. Consulta
`AUTOMATIC_FIRE_FRAME_LOCK.md` y `AUTOMATIC_FIRE_FRAME_LOCK_QA.txt`.

## v3.25.3 marco universal de apuntado

Arma visual, socket balístico y proyectiles lineales comparten el target de la
cruz y el mismo marco de cámara. Se mantiene el cálculo especializado de caída
para granadas y Bolt3D. Consulta `UNIVERSAL_AIM_FRAME_FIX.md`.

## v3.25.2 zeroing estable al caminar y con Gatling

El disparo conserva ahora un snapshot inmutable de la cámara del frame de
fuego, usa un socket balístico separado del muzzle visual animado y refresca
las raíces de actores antes de sincronizar equipamiento. Esto elimina la
reorientación hacia arriba o atrás causada por movimiento, recoil o eventos
recuperados. Consulta `ZEROING_MOTION_GATLING_FIX.md` y
`ZEROING_MOTION_GATLING_QA.txt`.

## GFaction89: player, allies, hostiles and neutral actors

`gfaction89` is now vendored as the relationship filter between Socketer's
omniscient candidate set and the existing GEDER/FPIL behavior stack. The
player is registered as an ordinary faction entity. Every actor may declare a
faction, team, role and tags; target legality and desirability come from
`config/factions/gameplay.ini`, not from an `enemy == player` assumption.

The startup scene includes a blue armed ally made from a capsule body and box
head. GWeapon89 selects its loadout, GAttach mounts the weapon object,
NationalMecanicanimal89 animates that object, and GFaction selects only legal
hostile targets. Hostiles may retarget the ally after it damages them.

```text
Socketer candidate positions
  -> GFaction relation/score filter
  -> NPC Eyes + Enlightener confirmation/interest
  -> GEDER accepted truth packet
  -> FPIL target-first verbs
  -> GWeapon/GAttach/mechanical animation
```

See `GFACTION89_ALLIED_TARGETS.md` and `GFACTION89_ALLIED_TARGETS_QA.txt`.

## NumBar preset orchestration v3.24.0

HUD meters now follow a reusable three-stage flow:

```text
GFO draw_numbar -> orchestrator INI -> visual .bhud preset -> GBar89
```

The player calls `config/hud/gameplay.ini`; the base `gameplay.bighud` keeps
only the ECG. Twenty reusable presets are included under
`config/hud/presets/`, covering radial, vertical, layered, masked, sprite,
vector-unit, state, overlay, mid-band, easing and full compositor styles. See
`DRAW_NUMBAR_ORCHESTRATOR.md` and `NUMBAR_PRESET_CATALOG.md`.


## Corrección visual v3.23.1

- `health_ring` vuelve a ser una barra circular continua (`radial_ring`).
- Conserva bevel, outline, sombra, extrusión, gloss y damage-lag intermedio.
- `health_vertical` usa 28 rectángulos horizontales reales.
- Se desactivaron `pixel_cells`, `pixel_quantize`, grids y scanlines en la barra vertical para impedir divisiones longitudinales.
- ECG y HUD de munición permanecen sin modificaciones.

## Base heredada de v3.23.0

- GBar89 anterior sustituido por GBar89 v0.4.
- BigVaderHudder expone compositor, frames, fondos, pixelado, middle band, unidades vectoriales y paridad radial/vertical.
- HUD gameplay actualizado para ejercitar health ring, barra vertical y munición vectorial.
- Sprite provider opcional y clip stack anidado.
- Prueba exhaustiva: `make test-gbar-v04-bighud`.

Consulta `GBAR89_V04_BIGHUD_FULL_SURFACE.md`.

## Casquillos gobernados por VPhysics

Esta revisión reemplaza la caída manual de los casquillos del weapon engine por
cuerpos rígidos ligeros dentro del único mundo VPhysics compartido. Las shells
ahora reciben impulso lineal y angular desde el `ejection_socket`, chocan contra
CCS/SICOL, rebotan según su perfil, pierden giro mediante damping y finalmente
entran en sleep.

```text
GWeapon89 CASING_EJECTED
        -> ejection transform del actor
        -> Blank3DCasingPhysics
        -> VPhysics dynamic box
        -> CCS/SICOL world contact
        -> quaternion/position writeback al renderer
```

También se corrigieron dos detalles del solver vendorizado: la restitución se
captura una sola vez antes de las iteraciones PGS y los impulsos correctivos de
un contacto en reposo ya no reinician continuamente el contador de sueño. Un
umbral de restitución evita microrebotes perpetuos a velocidades pequeñas.

Consulta `VPHYSICS_CASING_BOUNCE_FIX.md` y
`VPHYSICS_CASING_BOUNCE_QA.txt`.

## VPhysics Total Solver como servicio compartido

Esta revisión vendoriza `vphysics_total_solver_c89` y lo conecta como un solo
mundo físico compartido. Blank3D entrega al Total Solver dos puertos explícitos:

```text
Transform provider -> read/write world transform, move, rotate y scale
Collision provider -> CCS/SICOL contacts + host-driven CCD sweep/TOI
```

Los cuerpos con autoridad `PHYSICS` escriben su pose final de vuelta al mundo
de escena. Los nodos `EXTERNAL` reciben posición, rotación y escala desde el
host y conducen cuerpos cinemáticos. El scale permanece en la capa transform y
no se pierde cuando la física actualiza posición o rotación.

El runner incluye una prueba visible cerca de `z=18`: cuatro cuerpos dinámicos
y un beacon cinemático que demuestra `move + rotate + scale`. Controles:

```text
B  pausar/reactivar el servicio VPhysics
P  reiniciar el mundo de demostración
O  aplicar un impulso al cuerpo 70001
```

La prueba usa CCS/SICOL para suelo, escenario y barridos CCD. Los contactos
entre cuerpos VPhysics usan por ahora un proxy esférico conservador; el bridge
queda preparado para que un provider de shapes más rico reemplace esa parte sin
tocar el solver.

Consulta `VPHYSICS_TOTAL_PROVIDER_STACK.md` y
`VPHYSICS_TOTAL_PROVIDER_STACK_QA.txt`.


## Any actor, any weapon object

Esta revisión convierte el stack de GAttach en un servicio universal de
equipamiento. GWeapon89 selecciona por `actor_id` el arma activa; la sección
`[presentation]` del INI entrega el selector de modelo, socket, offset de agarre
y perfil mecánico; GAttach monta una instancia independiente del objeto; y
NationalMecanicanimal89 anima sus piezas sin saber si el portador es player,
enemigo, aliado o actor genérico.

`gunner_enemy` es la primera prueba integrada del runner y usa exactamente el
mismo contrato que el player. La prueba portable incluye también un aliado.
El `muzzle_socket` continúa separado de los efectos de muzzle.

Consulta `UNIVERSAL_ACTOR_WEAPON_ASSEMBLY.md` y
`UNIVERSAL_ACTOR_WEAPON_ASSEMBLY_QA.txt`.


## Attach first, animate second

Esta revisión vendoriza **GAttach89 v1.0.0** y corrige la frontera del arma
multipartes. El personaje ya no entrega su socket directamente al animador:
GAttach coloca un objeto hijo en un socket corporal nombrado y
NationalMecanicanimal89 anima después las piezas internas de ese objeto.

```text
personaje -> GAttach89 -> objeto equipado -> NationalMecanicanimal89
```

El bridge de GAttach es genérico para IDs arbitrarios de personajes, objetos y
sockets. El runner TPS demuestra el camino con el objeto `equipped_weapon` en
`weapon_r`; un caller puede crear más rigs para armas u objetos de NPC.

El fogonazo fue retirado de la geometría mecánica. El rig sólo expone un
`muzzle_socket` invisible y Blank3D lo republica mediante Soquete3D para que
las librerías independientes de muzzle, proyectiles, audio, gas, humo o luz lo
consuman. El animador conserva cuerpo, corredera, cargador, cañón, retroceso y
restricciones, pero no dibuja efectos de disparo.

Consulta `GATTACH_OBJECT_ANIMATOR_INTEGRATION.md`,
`MOTION_LIBRARY_SELECTION.md` y
`NATIONALMECANICANIMAL89_INTEGRATION.md` and
`GATTACH_OBJECT_ANIMATOR_QA.txt`.

Esta revisión conserva los enemigos y ataques de movimiento que ya funcionaban,
pero elimina la composición rígida del HUD normal. Ahora el archivo
`config/hud/gameplay.bighud` decide cómo se colocan, dibujan y enlazan:

- el ECG dinámico;
- barras lineales horizontales y verticales;
- barras segmentadas de munición;
- medidores circulares `radial_pie` y `radial_ring`;
- barras con lag, suavizado, estados, patrones, máscaras y capas;
- contadores numéricos de vida, cargador, reserva, BPM y amenaza.

El ECG y GBar89 siguen haciendo el dibujo especializado. BigVaderHudder es el
orquestador: lee el `.bighud`, crea los nodos, aplica cada parámetro y conecta
las variables reales del gameplay. No hace falta recompilar para rediseñar el
HUD; basta editar el archivo y volver a iniciar el ejecutable.


Esta revisión visual añade al nodo ECG:

- superficie de `360 × 160` para que la gráfica de escala vertical `3` se vea
  completa y no se corte por debajo;
- barrido clásico más rápido, aproximadamente `1.5 s` para recorrer las
  32 columnas a ritmo de reposo;
- salida RGBA con alfa independiente para contenido, caja de fondo y zona
  limpia;
- fondo semitransparente sin apagar el trazo, la rejilla ni los textos.

## Percepción target-first

Esta revisión vendoriza `3D_NPC_Eyes` y `EnlightenerAI` y los coloca alrededor
de GEDER. Socketer/Soquete3D sigue siendo la verdad absoluta T0; Eyes aporta
verdad visual T1 y EnlightenerAI aporta memoria, oído e interpretación T2. Si
las capas superiores fallan, GEDER conserva el target basal y FPIL puede usar
un comportamiento de fallback.

La gramática queda centrada en `target`: `rotatetotarget`, `lookattarget`,
`movetotarget` y `firetarget`. Los verbos FPSC antiguos como `rotatetoplr`,
`lookatplayer` y `shootplayer` sobreviven únicamente como alias compatibles.

La configuración sensorial ya no vive en la línea de spawn. Cada arquetipo
carga automáticamente la sección `[perception]` de
`config/entities/<archetype>.ini`; consulta `PERCEPTION_INI_CONFIGURATION.md`.
Los parámetros RPYL inline permanecen sólo como compatibilidad y override.

Consulta `SENSORY_TRUTH_STACK.md` para la lista completa de condiciones,
acciones y ejemplos FPIL. También se incluye
`scripts/sensory_enemy_example.fpi`.


## GEDER anexado al engine

Esta revisión vendoriza `GEDER Truth Gate Pipeline 1.0.0` y coloca una puerta
de verdad delante del tick FPIL de cada enemigo. El perfil y sus sensores se cargan ahora desde el INI del arquetipo. El preset
incluido usa `strict`, conserva Socketer como verdad basal y añade rango,
NPC Eyes, línea de visión y EnlightenerAI. El perfil estricto utiliza el
proveedor CCS/SICOL ya existente.

Consulta `GEDER_TRUTH_GATE_INTEGRATION.md` para la arquitectura, argumentos
RPYL, condiciones FPIL y razones terminales.

## Compilar en MSYS2 MinGW32

```bash
cd /d/MSYS2Portable/App/projects/blank3d_giffany_motion89_combat_traversal_v3_25_2_zeroing_motion_gatling
make clean
make
./blank3d.exe
```

Con consola de diagnóstico:

```bash
make debug
./blank3d_debug.exe
```

## Pruebas principales

```bash
make test-gfaction-vendor
make test-faction-bridge
make test-gattach-vendor
make test-attachment-stack
make test-nationalmecanicanimal-vendor
make test-mechanical-weapon
make test-npc-eyes-vendor
make test-enlightener-vendor
make test-perception-stack
make test-perception-ini
make test-geder-vendor
make test-truth-gate
make test-bighud
make test-ecg-vitals
make test-vphysics-provider
make test-casing-physics
make test-cameranaku-provider
make test-aim-convergence
make test-shotgun-runtime
make test-actor-equipment
make syntax-check
make audit
```

## Editar el HUD

Archivo principal:

```text
config/hud/gameplay.bighud
```

El preset incluido crea:

1. ECG clásico de 32 columnas con dinámica de vida/amenaza.
2. Aro circular de vida con daño retrasado y estados de color.
3. Barra vertical de vida.
4. Número de vida dentro del aro.
5. Barra segmentada de munición enlazada a la capacidad del arma actual.
6. Contador `cargador/reserva`.

Consulta `BIGHUD_ORCHESTRATION.md` para bindings, precedencia, parámetros y
ejemplos completos.

La integración se mantiene en C89 estricto, fixed-point donde corresponde,
almacenamiento estático/caller-owned y sin `malloc`, `realloc` ni `free`.

## v3.25.1 enemy/ally retarget correction

Hostile target selection now uses correctly converted Q16.16 distance, stable
tie-breaking and recent-damage retargeting. Enemies can choose the blue armed
ally instead of defaulting to the player. See `ENEMY_ALLY_RETARGET_FIX.md`.
