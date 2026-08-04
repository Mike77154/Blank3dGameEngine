# wsound_ggrenadeblast89 v1.0.2

Sintetizador procedural de explosiones de granadas y proyectiles de lanzagranadas.

- **C89 estricto**
- **fixed-point puro**
- **sin `malloc`, `realloc`, `calloc`, `free` ni heap**
- **sin `float` ni `double`**
- estado completo suministrado por el usuario
- licencia **CC0-1.0**
- salida mono PCM de 16 bits
- rango admitido: 8 kHz a 48 kHz; presets ajustados a 44.1 kHz

> Los WAV incluidos son previews de diseño sonoro, no mediciones físicas ni
> recreaciones calibradas en dB SPL. Escúchalos inicialmente a volumen bajo.

## Arquitectura sonora

```text
                  ┌─ Noise 0: estallido central, banda ancha filtrada
                  ├─ Noise 1: cuerpo una pseudo-octava grave
Disparo ─► ENV ───┼─ Noise 2: crack una pseudo-octava aguda
bipolar            ├─ Noise 3: fragmentos/chispas dispersas
Friedlander        ├─ Noise 4: tierra, polvo y escombros
                  ├─ Sine: presión grave con pitch-drop
                  └─ Saw: retumbar filtrado y modulado
                              │
                              ▼
                    EQ fijo de seis bandas
                              │
                    distorsión de mezcla paralela
                              │
                    chorus corto y poco profundo
                              │
                    reverb Schroeder fijo
                              │
                         PCM s16 mono
```

La envolvente principal representa tres rasgos útiles del frente de presión:

1. subida prácticamente inmediata;
2. fase positiva breve con caída rápida;
3. fase negativa de menor amplitud y mayor duración.

Las capas no intentan resolver dinámica de fluidos. Traducen propiedades
acústicas observables a un sintetizador de bajo costo: transitorio de banda
ancha, cuerpo grave, ensanchamiento temporal, reflexiones y dispersión.

## Los siete osciladores

| Oscilador | Papel |
|---|---|
| Noise 0 | estallido central filtrado |
| Noise 1 | versión grave por sample-and-hold y low-pass |
| Noise 2 | versión aguda por diferenciación/high-pass |
| Noise 3 | impulsos de fragmentación de densidad configurable |
| Noise 4 | cola de polvo, suelo y escombros |
| Sine | woofer principal con descenso de frecuencia |
| Saw | subgrave rugoso, redondeado por low-pass y modulado por ruido |

“Octava” en las capas de ruido significa **desplazamiento estadístico del
contenido espectral**, no afinación musical exacta: el ruido no posee una
fundamental única.

## Ecualizador de seis bandas

El banco usa cinco divisores low-pass y reconstruye seis regiones:

```text
0: < 90 Hz
1: 90–250 Hz
2: 250–700 Hz
3: 700–1800 Hz
4: 1.8–5 kHz
5: > 5 kHz
```

Las ganancias están en Q12 (`4096 = 1.0`).

## Presets

| ID | Constante | Intención |
|---:|---|---|
| 0 | `WS_GGB89_M67_OPEN` | granada de fragmentación en campo abierto |
| 1 | `WS_GGB89_40MM_HE_OPEN` | HE de 40 mm, golpe compacto |
| 2 | `WS_GGB89_40MM_HEDP_HARD` | impacto duro con fragmentación brillante |
| 3 | `WS_GGB89_INDOOR_CONFINED` | recinto con reflexiones y grave acumulado |
| 4 | `WS_GGB89_CONCRETE_IMPACT` | concreto, grava y metal secundarios |
| 5 | `WS_GGB89_DIRT_IMPACT` | suelo blando, agudos absorbidos |
| 6 | `WS_GGB89_DISTANT` | pulso ensanchado y filtrado por distancia |
| 7 | `WS_GGB89_ARCADE_HEAVY` | versión exagerada para videojuego |

## Uso mínimo

```c
#include "wsound_ggrenadeblast89.h"

static ws_ggb89 blast;

void audio_init(void)
{
    ws_ggb89_init(&blast, 44100UL, 0x12345678UL);
}

void grenade_impact(void)
{
    ws_ggb89_trigger(&blast, WS_GGB89_40MM_HEDP_HARD, 32767);
}

ws_gs16 audio_next_mono_sample(void)
{
    return ws_ggb89_process(&blast);
}
```

Para un bloque:

```c
static ws_gs16 block[512];

ws_ggb89_process_block(&blast, block, 512UL);
```

## Personalización

```c
ws_ggb89_params p;

ws_ggb89_get_preset(WS_GGB89_CONCRETE_IMPACT, &p);

/* Más cuerpo grave y menos vidrio. */
p.eq_gain_q12[0] = 7000;
p.eq_gain_q12[5] = 2200;

/* Chorus casi imperceptible para evitar el efecto "wiu-wiu". */
p.chorus_wet_q15 = 700;

/* Cola de recinto moderada. */
p.reverb_wet_q15 = 4500;
p.reverb_feedback_q15 = 24000;

ws_ggb89_trigger_custom(&blast, &p, 32767);
```

Todos los parámetros son enteros. La estructura `ws_ggb89_params` se puede
guardar en presets propios, INI parseado externamente o recursos del engine.

## Integración sin heap

`ws_ggb89` contiene todos los delays de chorus y reverb. Decláralo:

- como `static`;
- dentro de una arena fija;
- en un pool de voces preasignado;
- o como miembro de una entidad de audio persistente.

No conviene colocarlo como variable automática pequeña de función, porque cada
voz ocupa alrededor de 32.5 KiB con la configuración incluida.

## Construcción

Linux, BSD, macOS o MSYS2:

```sh
make
make check
make previews
```

MinGW32:

```bat
build_mingw32.bat
```

La validación usa:

```text
-std=c89 -pedantic -Wall -Wextra -Werror
```

## Distribución de archivos

```text
include/
  wsound_gtypes89.h
  wsound_geq6_89.h
  wsound_gdist89.h
  wsound_gchorus89.h
  wsound_greverb89.h
  wsound_ggrenadeblast89.h
src/
  módulos correspondientes
demo/
  render_previews.c
tests/
  test_c89.c
  test_transients.c
  test_speaker_safe.c
previews/
  ocho WAV corregidos, montaje y comparación A/B
docs/
  PHYSICS_NOTES.md
  TRANSIENT_FIX_REPORT.md
  SPEAKER_SAFE_FIX_REPORT.md
  CHANGELOG.md
  VALIDATION.txt
```

## Base física utilizada

- El perfil Friedlander idealiza un frente abrupto, fase positiva y fase
  negativa posterior.
- La propagación no lineal ensancha el pulso y desplaza el espectro hacia
  frecuencias menores con distancia.
- La reflexión y el confinamiento remodelan el historial presión-tiempo.
- Los impulsos explosivos contienen cuerpo grave, pero un frente más abrupto
  conserva energía de alta frecuencia.
- La fragmentación se modela como eventos cortos y dispersos, separada del
  frente de presión.

Referencias bibliográficas completas: `docs/PHYSICS_NOTES.md`.

## Licencia

SPDX-License-Identifier: CC0-1.0

Dedicado al dominio público mediante CC0 1.0 Universal.


## Corrección de crackle v1.0.2

Esta revisión corrige el crackle residual detectado al inicio de los presets
`40 mm HEDP hard impact` y `concrete impact`, visibles en el montaje alrededor
de 7–8 s y 14–15 s. Conserva fragmentos audibles, pero evita que cientos de
impulsos breves se acumulen como estática agresiva en bocinas pequeñas.

- Se reemplazó el clipping duro oculto antes del EQ por una rodilla suave fixed-point.
- El EQ de seis bandas también usa una salida de rodilla suave.
- Los eventos de fragmentación se evalúan a una tasa de control de 1/4 de la tasa de audio.
- Se impide retrigger mientras el pulso anterior todavía es fuerte.
- La amplitud de cada fragmento varía de forma determinista.
- La fuente de fragmentos usa dos etapas low-pass fixed-point.
- La envolvente de ataque es más lenta para evitar aristas de una muestra.
- Se añadió una prueba específica de aspereza/segunda diferencia para los presets 2 y 4.
- No se agregó heap, punto flotante ni dependencia matemática en runtime.
