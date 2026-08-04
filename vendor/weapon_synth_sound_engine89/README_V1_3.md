# Weapon Synth Sound Engine 89 v1.3.0 — Fire Tune + Ordnance

Motor modular de sonido procedural para armas, mecanismos, proyectiles, impactos,
casquillos, fuego, granadas y cohetes. Esta revisión corrige el hiss continuo del
lanzallamas e integra tres proveedores nuevos sin fusionar ni modificar sus APIs:

- `gbulletair89 v1.2` — balas cortando el aire, whiz, zip y crack;
- `wsound_ggrenadeblast89 v1.0.2` — explosiones de granadas;
- `wsound_rocketblast89 v1.3.0` — lanzamiento, vuelo e impacto de cohetes.

El puente `gsynthordnance89` es opcional. Las tres librerías originales permanecen
independientes y también pueden usarse sin el resto del engine.

## Protocolo

- C89 estricto;
- fixed-point / enteros únicamente en los cores;
- sin `malloc`, `calloc`, `realloc` ni `free`;
- sin heap interno;
- sin `float`, `double` ni `math.h`;
- pools y delays proporcionados por el caller;
- Voice Handler con 256 voces lógicas / 64 físicas por defecto;
- build validado con `-std=c89 -pedantic -Wall -Wextra -Werror -O2`;
- objetos i386 de los cores nuevos: PASS.

## Corrección del lanzallamas

El preset anterior tenía demasiado airflow y brillo, poco cuerpo y dos chorros casi
idénticos superpuestos. La nueva afinación usa:

```text
menos airflow / hiss
menos brightness
más pressure y low body
más turbulencia modulada
más crackle
variación correlacionada por semilla
reducción automática de una segunda llama activa
solapamiento temporal mínimo en el showcase
```

Medición exacta en el tramo 7.0–10.0 s, comparado con el demo v1.1:

```text
RMS total:        -38.2 %
20–250 Hz:         -1.7 %  (cuerpo prácticamente intacto)
4–16 kHz:         -52.6 %
8–18 kHz:         -61.5 %
pico:              30895 -> 23562
clipping:          0 muestras
```

## Arquitectura nueva

```text
Weapon Synth Sound Engine
│
├─ gsynthsoundengine89
│  ├─ gtinkle89        -> CASING
│  └─ gfire89 v1.2     -> AMBIENCE / EXPLOSION
│
├─ gsynthordnance89
│  ├─ gbulletair89     -> RICOCHET / ballistic detail
│  ├─ grenadeblast89   -> EXPLOSION + CRITICAL
│  └─ rocketblast89    -> EXPLOSION + CRITICAL
│
└─ gweaponvoice89 / gvoice89
   ├─ pan, distancia y prioridad
   ├─ voice steal
   ├─ virtualización
   └─ mezcla estéreo y limitador
```

### Particularidades del puente

- `gbulletair89` trabaja internamente a 48 kHz; el proveedor usa resampling lineal
  Q16 para el engine a 44.1 kHz, sin coma flotante.
- Granadas y cohetes se registran como voces críticas y no virtualizables durante
  su transitorio principal.
- El core estéreo de cohetes se reduce a una fuente mono antes del Voice Handler;
  el paneo y la distancia siguen perteneciendo al sistema espacial del engine.
- Todos los pools son caller-owned y tienen capacidad configurable.

## Memoria del puente de munición pesada

Medido en el ABI de 64 bits del entorno de validación:

```text
gsso89_context                 56 B
cada voz bullet             4,904 B
cada voz grenade           33,352 B
cada voz rocket            48,552 B

pool sugerido 32/4/2      387,496 B  (~378.4 KiB)
```

El juego puede usar pools mucho menores. Por ejemplo, 8 balas de aire, 2 granadas
y 1 cohete consumen aproximadamente 154 KiB.

## Demos principales

```text
audio/06_flamethrower_corrected_dual.wav
audio/07_gbulletair89_isolated_catalog.wav
audio/08_grenadeblast89_isolated_catalog.wav
audio/09_rocketblast89_isolated_catalog.wav
audio/10_ordnance_integrated_scene.wav
audio/11_full_weapon_engine_plus_ordnance.wav
audio/14_AB_old_hiss_then_corrected_7_10.wav
audio/16_AB_corrected_engine_then_full_ordnance.wav
```

`00_v1_3_release_showcase.wav` concatena la corrección del fuego, los tres
catálogos independientes, la escena integrada y el engine completo.

## API mínima de `gsynthordnance89`

```c
#include "gsynthordnance89.h"

static gsso89_bullet_voice bullets[16];
static gsso89_grenade_voice grenades[4];
static gsso89_rocket_voice rockets[2];
static gsso89_context ordnance;

gsso89_init(&ordnance,
            bullets, 16U,
            grenades, 4U,
            rockets, 2U,
            44100U);
```

Después se usan `gsso89_play_bullet`, `gsso89_play_grenade` o
`gsso89_play_rocket` contra un `gwv89_context` ya inicializado.

## Compilar

```sh
make
make test
make render
```

## Artefactos de validación

- `FIRE_HISS_FIX_REPORT.md`
- `ORDNANCE_INTEGRATION_REPORT.md`
- `BUILD_REPORT_V1_3.txt`
- `PROTOCOL_SCAN_V1_3.txt`
- `I386_BUILD_V1_3.txt`
- `WAV_VALIDATION_V1_3.json`
- `FIRE_HISS_METRICS_V1_3.json`
- `SHA256SUMS_V1_3.txt`

Las licencias originales permanecen dentro de cada directorio vendorizado.
