# Fase 1 + Fase 2 acústica — weapon_synth_sound_engine89 v1.5.0

## Objetivo

Esta versión añade un **stack acústico físico/perceptual** a la síntesis procedural existente. No reemplaza los sintetizadores de arma: procesa su resultado según la presión inicial, la trayectoria por el aire y la respuesta del escenario.

```text
SINTETIZADOR DEL ARMA / PROYECTIL / IMPACTO
                    │
                    ├── pulso de presión bipolar
                    ├── armónico grave de traducción
                    ▼
              DIRECT PATH
       distancia + aire por bandas
       + directividad por fuente
       + oclusión/transmisión/material
       + portal
                    │
           ┌────────┴────────┐
           ▼                 ▼
 EARLY REFLECTIONS      LATE REVERB
 8 taps por bandas      4 líneas con damping
           └────────┬────────┘
                    ▼
     COMBAT BUS MULTIBANDA + LIMITER
                    ▼
                 PCM16
```

## Fase 1 — impacto físico y perceptual

### 1. Pulso de presión bipolar

`wsounda89_trigger_pressure()` genera una presión positiva abrupta, seguida por una fase negativa menor. Hay perfiles independientes para reporte, impacto, granada y cohete. La fachada de eventos lo activa automáticamente al recibir esos eventos, aunque también puede dispararse manualmente con `WSSE89_EVENT_PRESSURE_TRIGGER`.

### 2. Propagación low/mid/high

El camino directo se divide en tres bandas y aplica por separado:

- retardo por distancia y velocidad de propagación;
- atenuación geométrica;
- absorción atmosférica suministrada por el host;
- directividad dependiente de la clase de fuente;
- oclusión;
- transmisión por material;
- transmisión/apertura de portal.

### 3. Directividad por clase de fuente

Clases públicas:

```text
REPORT, BODY, GAS, MECHANISM,
PROJECTILE, IMPACT, EXPLOSION, TAIL
```

La fachada cambia automáticamente la clase al despachar reportes, proyectiles, impactos, ricochets, granadas y cohetes. El host controla la orientación mediante `source_dot_q15`.

### 4. Combat bus multibanda

El master se divide en low/mid/high. Cada banda conserva su propio detector y ganancia dinámica. Las reflexiones y la cola se duckean durante frentes intensos, mientras el transitorio directo conserva definición. Un limitador final evita saturación PCM16.

### 5. Armónico de traducción

La energía grave produce un componente armónico rectificado, con eliminación de DC. Esto mantiene sensación de peso en altavoces pequeños sin convertir el subgrave en una nota tonal independiente.

## Fase 2 — escenario acústico

### 6. Reflexiones tempranas

Ocho taps configurados por espacio, cada uno con:

- retardo;
- ganancia low/mid/high;
- paneo;
- absorción derivada del material.

### 7. Reverberación tardía

Cuatro líneas de feedback con longitudes no coincidentes, damping y paneo distintos. El material modifica feedback y pérdida de agudos. Con memoria pequeña, los taps largos se acortan de manera segura en vez de desactivar el stack.

### 8. Materiales

Presets incluidos:

```text
AIR, CONCRETE, BRICK, WOOD, DRYWALL,
GLASS, METAL, SOIL, VEGETATION,
WATER, FABRIC
```

Cada material tiene absorción, transmisión low/mid/high y scattering. `thickness_q15` interpola la transmisión desde aire libre hasta el preset del material.

### 9. Exterior y ground bounce

Los espacios FIELD, FOREST y URBAN añaden una reflexión de suelo con polaridad, retardo y pérdida de agudos propios. MOUNTAIN deja ecos discretos largos. El host puede cambiar la distancia, el material del suelo y la orientación sin recompilar.

### 10. Portales

El portal añade:

- retardo extra;
- apertura Q15;
- transmisión low/mid/high;
- paneo aparente de la abertura.

Esto permite que una fuente ocluida parezca llegar desde una puerta, ventana o pasillo.

## Perfiles

```text
WSOUNDA89_REALISTIC
    presión y traducción moderadas; mayor contraste dinámico.

WSOUNDA89_HYBRID
    física dominante con refuerzo perceptual; perfil predeterminado.

WSOUNDA89_CINEMATIC
    más traducción grave, early/late y control dinámico.
```

## Uso por la fachada universal

```c
wsse89_event e;
wsounda89_path_params path;
wsounda89_portal_params portal;

wsounda89_path_defaults(&path);
path.distance_cm = 2800U;
path.speed_cm_s = 34300U;
path.source_dot_q15 = 24500;
path.occlusion_q15 = 27000U;
path.source = WSOUNDA89_SOURCE_REPORT;
path.air_absorb_q15[0] = 120U;
path.air_absorb_q15[1] = 500U;
path.air_absorb_q15[2] = 1800U;

e.type = WSSE89_EVENT_ACOUSTIC_PROFILE;
e.data.acoustic_profile = WSOUNDA89_HYBRID;
wsse89_dispatch(&engine, &e, 0);

e.type = WSSE89_EVENT_ACOUSTIC_SPACE;
e.data.acoustic_space = WSOUNDA89_SPACE_URBAN;
wsse89_dispatch(&engine, &e, 0);

e.type = WSSE89_EVENT_ACOUSTIC_MATERIAL;
e.data.acoustic_material.material = WSOUNDA89_MATERIAL_BRICK;
e.data.acoustic_material.thickness_q15 = 24500U;
wsse89_dispatch(&engine, &e, 0);

e.type = WSSE89_EVENT_ACOUSTIC_PATH;
e.data.acoustic_path = path;
wsse89_dispatch(&engine, &e, 0);

wsounda89_portal_defaults(&portal);
portal.delay_samples = 220U;
portal.opening_q15 = 18000U;
portal.transmit_q15[0] = 27000U;
portal.transmit_q15[1] = 20500U;
portal.transmit_q15[2] = 11000U;
portal.pan_q15 = -12000;

e.type = WSSE89_EVENT_ACOUSTIC_PORTAL;
e.data.acoustic_portal = portal;
wsse89_dispatch(&engine, &e, 0);
```

Después, los eventos normales `REPORT`, `IMPACT`, `GRENADE_BLAST` y `ROCKET_BLAST` activan automáticamente el pulso y la clase directiva correspondiente.

## Integración verdaderamente por fuente

La fachada unificada aplica el stack sobre la mezcla del mundo y sirve como integración sencilla o fallback. Para múltiples fuentes simultáneas con posiciones/materiales diferentes, el game engine debe reservar **un `wsounda89_context` por fuente, grupo o bus acústico**, procesar cada voz antes de mezclarla y reutilizar contextos mediante un pool estático.

```text
voice A → acoustic context A ┐
voice B → acoustic context B ├→ mixer del host → master
voice C → acoustic context C ┘
```

No existe ray tracer interno. El host aporta distancia, orientación, oclusión, material, portal y espacio desde sus propias colisiones, BSP, raycasts, rooms o sistema de navegación. Por eso el módulo continúa siendo agnóstico a cualquier game engine.

## Memoria

Toda la memoria es caller-owned.

- `wsounda89_required_direct_samples(rate, max_distance_cm, speed_cm_s)` calcula el delay directo requerido.
- `wsounda89_required_world_samples(rate)` devuelve el mínimo para funcionar.
- Un buffer world mayor conserva los ecos largos completos; uno menor los acorta con degradación elegante.
- No usa heap ni asignación dinámica.

## Base técnica consultada

El diseño sigue la separación usada por motores acústicos modernos entre camino directo, atenuación por distancia, absorción del aire, directividad, oclusión y transmisión por bandas. También separa reflexiones tempranas de reverberación tardía y usa materiales con absorción/transmisión por bandas. Como referencias de diseño se consultaron:

- Valve Steam Audio — Simulation y Materials.
- Apple PHASE — Geometry-aware audio.
- ISO 9613-1 — Atmospheric absorption.
- ISO 9613-2:2024 — Outdoor sound propagation.

Los coeficientes incluidos son presets prácticos en Q15, no una implementación certificada de ISO ni una simulación geométrica completa.
