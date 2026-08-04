# Blank3D v3.18.1 — Sensory Truth Stack + Perception INI

## Propósito

La percepción de un NPC ya no se reduce a `player visible = sí/no`. Cada actor
construye una pila de verdades acerca de su **target actual**:

```text
Socketer / Soquete3D
  T0: verdad absoluta y basal
  identidad + pose exacta del target
            |
            v
3D_NPC_Eyes
  T1: verdad percibida
  forma visual + rango + FOV + muestras + oclusión
            |
            v
EnlightenerAI
  T2: verdad interpretada
  oído + memoria + última posición + sospecha + alerta
            |
            v
GEDER Truth Gate
  acumula las verdades disponibles sin permitir que una capa opcional
  destruya la verdad basal de Socketer
            |
            v
FPIL
  pregunta por el nivel de evidencia y ordena una acción
```

Si Eyes o EnlightenerAI no aportan nada, GEDER conserva T0. Por eso el NPC
puede degradar su comportamiento sin quedarse lógicamente ciego:

```text
visión actual       -> usa posición visible
sin visión + memoria-> usa última posición vista
sin visión + sonido -> usa última posición oída
sin capas superiores-> fallback a la pose absoluta de Socketer
```

## Gramática FPIL

Se conserva la forma clásica inspirada en FPSC:

```text
:condicion1,condicion2:accion1,accion2
```

Todos los verbos se expresan desde el punto de vista del NPC que ejecuta el
script. El concepto central es `target`, no `player`.

## Condiciones de target basal

| Condición | Significado |
|---|---|
| `hastarget` | Socketer tiene una pose válida para el target. |
| `targetknown` | Alias de `hastarget`. |
| `socketertruth` | Existe la verdad absoluta T0. |
| `targetacceptedastruth` | GEDER conserva al menos la verdad basal. |
| `targetalive` | El target continúa activo. |
| `targetdistwithin=N` | Distancia 3D al target menor o igual a N. |
| `targetdistfurther=N` | Distancia 3D mayor que N. |
| `targetflatdistwithin=N` | Distancia XZ menor o igual a N. |
| `targetflatdistfurther=N` | Distancia XZ mayor que N. |

Los nombres antiguos `plrdistwithin`, `plrdistfurther`,
`plrflatdistwithin` y `playerflatdistwithin` permanecen como alias.

## Condiciones de 3D_NPC_Eyes

| Condición | Significado |
|---|---|
| `targetvisible` | Al menos una muestra visual está despejada. |
| `targetcanbeseen` | Alias de `targetvisible`. |
| `targetfullyvisible` | Todas las muestras necesarias dan visibilidad completa. |
| `targetpartial` | Sólo parte del volumen del target es visible. |
| `targetoccluded` | El target cae en la forma visual, pero una geometría lo tapa. |
| `targetoutofrange` | Está más allá del rango visual. |
| `targetoutofshape` | Está fuera del cono, esfera, caja o frustum. |
| `targetraysclearatleast=N` | Al menos N rayos de muestra llegaron al target. |
| `canperceiveplayer` | Alias legado de visión actual, no de verdad absoluta. |

## Condiciones de EnlightenerAI

| Condición | Significado |
|---|---|
| `targetheard` / `noiseheard` | El target produjo un sonido útil. |
| `targetremembered` | Existe una última posición visual todavía vigente. |
| `targetinferred` | No se ve ahora, pero oído o memoria permiten inferirlo. |
| `targetseenfor=N` | Tiempo visible acumulado de al menos N segundos. |
| `targetlostfor=N` | Tiempo sin visión de al menos N segundos. |
| `alertnessatleast=N` | Alerta de 0 a 100. |
| `suspicionatleast=N` | Sospecha de 0 a 100. |

## Condiciones GEDER y metapercepción

| Condición | Significado |
|---|---|
| `truthaccepted` | La evaluación GEDER terminó aceptada. |
| `truthrejected` | La evaluación no fue aceptada. |
| `truthfallbackactive` | Sólo queda la verdad basal de Socketer. |
| `truthcountatleast=N` | GEDER acumuló al menos N verdades. |
| `truthscoreatleast=N` | Puntuación entera mínima. |
| `truthprofileis=NAME` | Perfil `retro`, `active`, `proximity` o `strict`. |
| `truthlevelatleast=absolute` | Tiene T0. |
| `truthlevelatleast=perceived` | Tiene T1 o superior. |
| `truthlevelatleast=enlightened` | Tiene T2. |
| `truthsourcehas=socketer` | T0 presente. |
| `truthsourcehas=eyes` | Verdad visual presente. |
| `truthsourcehas=enlightener` | Interpretación presente. |
| `truthsourcehas=hearing` | Contribución auditiva presente. |
| `truthsourcehas=memory` | Contribución de memoria presente. |
| `targetconfidenceatleast=N` | Confianza de acción de 0 a 100. |

## Acciones canónicas

| Acción | Comportamiento |
|---|---|
| `settarget=player` | Selecciona al jugador como target actual. |
| `cleartarget` | Borra el target del agente. |
| `rotatetotarget` | Gira el cuerpo hacia la mejor posición del target. |
| `lookattarget` | Alias corporal actual; preparado para cabeza/esqueleto futuro. |
| `movetotarget=N` | Avanza hacia la mejor posición a velocidad N. |
| `runtotarget=N` | Alias semántico de movimiento rápido. |
| `firetarget` / `shoottarget` | Dispara hacia la mejor posición disponible. |
| `rotatetolastseen` | Gira a la última posición vista. |
| `movetolastseen=N` | Investiga la última posición vista. |
| `rotatetolastsound` | Gira hacia la última posición oída. |
| `movetolastsound=N` | Investiga el último sonido. |
| `seteyeshape=NAME` | `cone`, `sphere`, `box` o `frustum`. |
| `setviewrange=N` | Cambia el alcance visual y el rango GEDER. |
| `setviewfov=N` | Cambia FOV horizontal en grados. |
| `setverticalfov=N` | Cambia FOV vertical. |
| `sethearingrange=N` | Cambia alcance auditivo. |
| `setrequirelos=0|1` | Activa/desactiva raycast de oclusión. |

### Compatibilidad FPSC

Los verbos centrados en player se normalizan al target actual:

```text
rotatetoplr  -> rotatetotarget
lookatplayer -> lookattarget
fireplayer   -> firetarget
shootplayer  -> shoottarget
movefore     -> movetotarget
```

No existe una ruta de IA distinta para `rotatetoplr`: es únicamente un alias
para que scripts antiguos sigan cargando.

## Configuración sensorial por INI

El camino normal ya no añade una cola de parámetros al spawn. El arquetipo
`zombie` carga automáticamente:

```text
config/entities/zombie.ini
```

La misma regla se aplica a `gunner_enemy`, `hopper_enemy`, `dive_enemy`,
`air_lunger_enemy` y `ground_lancer_enemy`.

```ini
[perception]
truth_profile=strict
range=30
eye_shape=cone
horizontal_fov=100
vertical_fov=70
hearing_range=24
require_line_of_sight=true
```

`range` alimenta tanto el rango visual de NPC Eyes como el filtro de distancia
de GEDER. Para separarlos pueden usarse secciones explícitas:

```ini
[truth]
profile=strict
range=36

[eyes]
shape=cone
range=30
horizontal_fov=100
vertical_fov=70
require_line_of_sight=true

[hearing]
range=24
```

Los nombres inline anteriores siguen aceptados como override de compatibilidad:
`truthprofile`, `truthrange`, `perceptionrange`, `eyeshape`, `viewfov`,
`verticalfov`, `hearingrange` y `requirelos`. También puede indicarse un INI
alternativo con `perceptionini ruta.ini` o `perception ruta.ini`.

## Ejemplo FPIL sensorial

```text
:state=0,targetvisible,targetdistwithin=28:setstate=1
:state=0,targetheard:rotatetolastsound,setstate=2

:state=1,targetfullyvisible,targetdistwithin=14:rotatetotarget,firetarget
:state=1,targetpartial:rotatetotarget,movetotarget=2.2
:state=1,targetremembered:rotatetolastseen,movetolastseen=2

:state=2,targetheard:rotatetolastsound,movetolastsound=1.8
:state=2,targetvisible:setstate=1

; Las capas superiores fallaron: GEDER vuelve a T0/Socketer.
:truthfallbackactive:rotatetotarget,movetotarget=1.2
```

## Implementación

```text
vendor/3d_npc_eyes/
vendor/enlightenerai/
vendor/geder_truth_gate_pipeline/
src/blank3d_perception.h
src/blank3d_perception.c
src/blank3d_perception_ini.h
src/blank3d_perception_ini.c
src/blank3d_truth_gate.h
src/blank3d_truth_gate.c
```

El estado es caller-owned/estático. La integración usa C89 y fixed-point, sin
`malloc`, `realloc`, `free`, `float` ni `double` en el camino de simulación.
