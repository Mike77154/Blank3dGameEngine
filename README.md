# Blank3D v3.27.3 — Corte de katana completamente delante del jugador

## v3.27.3 pivote frontal y plano de seguridad

La katana gigante y su OBB roja ya no giran alrededor de un pivote metido en
el torso. El socket independiente `player.melee_r` toma su posición desde
`config/melee/katana.ini` y desplaza **todo el rig Mecanim** 1.90 m hacia el
frente local del jugador. Como la mesh, la hoja animada y la OBB ofensiva
comparten el mismo root, el corte visible y el daño permanecen juntos delante
del personaje.

La regresión recorre la animación completa y comprueba dos cosas por separado:
que el AABB conservador de la OBB roja no cruce el plano corporal y que ningún
vértice de la katana visible vuelva a pasar por el protagonista. Las hurtboxes
enemigas continúan como cajas verdes de torso y cabeza. Consulta
`KATANA_FRONT_ONLY_SWING.md` y `KATANA_FRONT_ONLY_SWING_QA.txt`.

## v3.27.1 OBB auténtica y primer montaje frontal de la katana

`katana89` genera una katana procedural visible y `NationalMecanicanimal89`
anima un corte activado con `K`. La colisión ofensiva sólo existe durante los
frames o milisegundos declarados en `config/melee/katana.ini`; el núcleo
`hurtbox3d + hitbox3d + melee3d` del paquete PhysicalDamageCollision3D cruza
una primitiva OBB real, orientada por la matriz de la hoja, contra los
volúmenes vulnerables de enemigos. La OBB conserva su pose anterior para cubrir el
barrido entre frames.

En esa versión la katana quedó centrada 0.80 m delante del actor; v3.27.3
reemplaza ese primer montaje por el pivote frontal configurable de 1.90 m. `F3` alterna el dibujo de depuración: rojo para la
caja ofensiva y verde para los volúmenes vulnerables. El prototipo no entra en
`GWeapon89`, no altera balística y no afecta a las armas de NPC. Consulta
`KATANA_MECANIM_PHYSICAL_DAMAGE_PROTOTYPE.md`.


## v3.26.2 proyectiles físicos del jugador guiados por el target de cámara

Lanzagranadas, rocket launcher y resortera ya no usan el plano de zeroing
antiguo de 32 unidades. La cámara selecciona el objetivo real de la retícula;
el proyectil nace en su muzzle físico y apunta a ese punto. Rocket conserva
trayectoria lineal, mientras granada y Bolt3D calculan su arco después. La ruta
es exclusiva del jugador y no modifica a los NPC. Consulta
`PLAYER_PHYSICAL_PROJECTILE_AIM.md`.


## v3.26.1 proyectil y trace visibles; raycast amarillo oculto

Las armas lineales del jugador conservan el raycast de cámara/muzzle como
autoridad inmediata de impacto, pero ese rayo ya no se manda al renderer. En
su lugar se crea un proyectil cosmético móvil, con mesh y Aoi Trail3D, que
recorre desde el muzzle visual hasta el punto ya resuelto. No tiene daño,
colisión ni influencia sobre NPCs. Si el proveedor de trails está lleno, sólo
se dibuja el pequeño tramo recorrido durante el frame, nunca la línea completa
de origen a impacto. Consulta `PLAYER_PROJECTILE_VISUALS.md`.


## v3.26.0 una cámara por archivo INI

Cameranaku descubre todas las cámaras habilitadas en `config/cameras/`.
`TPS Centred`, `OTS` y `FPS` ya son perfiles completamente separados; `V`
recorre el catálogo ordenado y se pueden agregar más modos copiando la
plantilla `camera_profile.ini.example`, cambiando su `id` y guardándola como
`.ini`. La carpeta y el perfil inicial se eligen desde `config/blank3d.toml`.

No hay una enumeración rígida de tres modos y los perfiles sólo gobiernan al
jugador; los NPC conservan su ruta de cámara/armas. Consulta
`CAMERA_INI_CATALOG.md`.

## v3.25.5 autoridad de raycast de cámara para el jugador

Las armas lineales del jugador ya no usan un proyectil físico persistente para
decidir el impacto. FPS dispara un raycast desde la cámara al centro del
viewport. TPS y OTS seleccionan el objetivo con ese mismo rayo y después hacen
una segunda comprobación desde el muzzle para evitar disparar a través de una
cobertura cercana. El daño es inmediato. Desde v3.26.1 la presentación usa un
proyectil cosmético móvil con mesh y trail, sin colisión ni daño.

El cambio no se aplica a NPCs. Granada, cohete y Bolt3D continúan siendo
proyectiles físicos. La cámara anterior se dividió en `TPS` centrada, `OTS` con
offset de hombro y `FPS`; `V` recorre esos tres modos. Consulta
`PLAYER_CAMERA_RAY_AUTHORITY.md`, `TPS_OTS_CAMERA_SPLIT.md` y
`PLAYER_CAMERA_RAY_QA.txt`.

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

## input_keys89 vendor layer

Keyboard name/alias identity is now isolated in `vendor/input_keys89`.
Blank3D resolves DDSL2 key names through this stateless C89 library before
querying HID-backed scanner state. The library owns no gameplay semantics, so
the wildcard key assigner remains unrestricted.

Validation targets:

```sh
make test-input-keys-vendor
make test-input-stack
make syntax-check
```

## Runtime spine ontology

The World3D89 + Scene3D89 + GFO Object + ThingSystem89 + ECS89 + ActorSystem89 integration is documented in `RUNTIME_SPINE_WORLD_SCENE_GFO_THING_ECS_ACTOR.md`. The key rule is large managers once per runtime/definition, lightweight handles per Thing/Entity/Actor.

## Universal variable authoring

The runtime now includes `var_dsl89 + var_runtime89 + var_manager89` as a
GameMaker-like authoring surface routed to NumSys, Flags or dynamic VarStore
without replacing any specialized system. See `VAR_DSL89_UNIVERSAL_AUTHORING.md`.

## GLOCO89 locomotion provider spine

`gloco89` is vendored as the character locomotion controller between movement
intent and the host physics/transform authority. MovementBaseVerbs and flat
GAutomotion paths feed it; VerticalMotion, Collision and VPhysics resolve the
result; World3D, Scene3D and Soquete T0 receive the final host transform.
Stamina is provider-routed to NumSys and `locomotion.*` profile/state fields are
available through the universal VarRuntime authoring layer. Special MotionAttack
modes can suspend GLOCO rather than competing for transform authority.

Validation targets:

```sh
make test-gloco-vendor
make test-gloco-provider-stack
make syntax-check
make syntax-check-input-win32
make audit
```

See `GLOCO89_LOCOMOTION_PROVIDER_INTEGRATION.md`.

## Condor Event / Condition / Action rule core

This build adds the context-based `condor_evact89` rule engine and the
`Blank3DCondor` bridge. `gameverbs89` remains the named DSL vocabulary; Condor
adds persistent/reactive `event -> condition -> action` rules. See
`CONDOR_EVACT89_RULE_ENGINE.md` and `CONDOR_EVACT89_QA.txt`.

## Mount89 white-box vehicle prototype

A white box at `(5, 0.6, 6)` exercises the vendored `3d_mounting_system89`.
Walk close and press **M** to mount/dismount. While mounted, the existing
movement verbs drive the box at 16 u/s (19 u/s run) instead of moving the
7 u/s player directly. See `MOUNT89_BOX_CAR_PROTOTYPE.md`.

## Buster-style multi-projectile charge selector

The Weapon System now vendors `morethanone89`. A `press_charge_release` weapon
can fire its normal projectile immediately on press, then select a different
projectile recipe by hold duration and emit exactly one charged shot on
release. See `MORETHANONE89_BUSTER.md` and `config/weapons/buster.ini`.

## SpriteAsset89 / SpriteVerbs89 / AssetRoute89 pipeline

Blank3D now vendors a provider-driven sprite asset layer. `AssetRoute89`
resolves logical asset names, `SpriteAsset89` owns clips/frames/playback, and
`SpriteVerbs89` exposes Ren'Py/GameMaker-like semantic commands to RPYL/DDSL2.
The bridge reuses `Blank3DImageAssets + imgcc0` for decoding/upload and can draw
sprite-sheet subrectangles through the existing OpenGL host. Existing
SpritePlane89/HUD/muzzle image paths retain their old direct-path behavior and
can additionally resolve logical routed names.

See `SPRITE_ASSET_PIPELINE89_INTEGRATION.md` and
`SPRITE_ASSET_PIPELINE89_QA.txt`.

Validation targets:

```sh
make test-spriteasset89-vendor
make test-spriteverbs89-vendor
make test-assetroute89-vendor
make test-sprite-runtime89
make test-image-stack
make test-muzzle-image-pipeline
make syntax-check
make syntax-check-sprite-runtime89-win32
```

## Flamethrower FireLoop billboard experiment

Weapon ID 14 now keeps its `expandiblefire89` boxes as mechanical collision
truth but skins them with the user-supplied FireLoop1 128x128/50-frame atlas.
The card is camera-facing, animated per projectile, three-layer additive, and
feeds a separate GL_LIGHT2 stream illumination pulse. See
`FLAMETHROWER_FIRELOOP_BILLBOARD.md`.
