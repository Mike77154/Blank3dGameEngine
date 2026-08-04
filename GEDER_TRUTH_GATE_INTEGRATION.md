# GEDER Truth Gate Pipeline — integración Blank3D v3.17.2

## Objetivo

GEDER fue anexado como una biblioteca agnóstica. El engine no reescribe su
algoritmo ni lo mezcla con FPIL: conserva el paquete original dentro de
`vendor/geder_truth_gate_pipeline` y usa un bridge pequeño en
`src/blank3d_truth_gate.c`.

```text
hechos del mundo
    │
    ▼
Blank3DTruthFacts
    │
    ▼
GEDER_Pipeline
    ├── REJECT   -> no ejecutar el tick FPIL del enemigo
    ├── ACCEPT   -> ejecutar el tick FPIL del enemigo
    └── CONTINUE -> acumular verdad y consultar la puerta siguiente
```

El perfil predeterminado es `retro`. Acepta inmediatamente y, por tanto,
conserva exactamente el comportamiento de los enemigos que ya funcionaban.
No se cambió `enemy.fpi` ni los scripts particulares de los arquetipos.

## Directorios

```text
vendor/geder_truth_gate_pipeline/
├── include/geder_truth_gate.h
├── src/geder_truth_gate.c
├── tests/test_geder_truth_gate.c
├── examples/
├── Makefile
├── README.md
└── LICENSE.txt

src/
├── blank3d_truth_gate.h
└── blank3d_truth_gate.c

tests/
└── test_truth_gate.c
```

## Perfiles disponibles

| Perfil | Puertas en orden | Resultado práctico |
|---|---|---|
| `retro` | aceptación inmediata | comportamiento anterior sin filtros |
| `active` | enemigo vivo, jugador vivo | detiene la lógica al caer el jugador |
| `proximity` | vivo, jugador vivo, conocimiento, distancia | el FPIL sólo corre dentro del rango |
| `strict` | lo anterior más raycast de línea de visión | percepción con oclusión del escenario |

Cada puerta aprobada acumula una verdad, un punto Q16.16 y un bit en la máscara
de GEDER. Una puerta fallida corta el pipeline inmediatamente.

La línea de visión del perfil `strict` consulta el proveedor de colisión ya
existente mediante `blank3d_collision_raycast()` y la capa
`B3D_COLLISION_LAYER_WORLD`.

## Configuración por arquetipo desde INI

Cada enemigo toma su perfil GEDER y sus sensores del archivo asociado:

```text
config/entities/<archetype>.ini
```

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

La clave `range` sincroniza el rango de verdad con el rango visual. Las
secciones `[truth]`, `[eyes]` y `[hearing]` permiten separarlos. Los argumentos
RPYL inline siguen soportados únicamente para compatibilidad u overrides de
instancia.

## Condiciones FPIL expuestas

El bridge publica el último acumulador GEDER para scripts y diagnóstico:

```text
truthaccepted
playeracceptedastruth
canperceiveplayer
truthcountatleast=N
truthscoreatleast=N
truthreason=N
truthprofileis=retro|active|proximity|strict
```

Cuando el pipeline rechaza, el host omite ese tick FPIL. El estado del enemigo,
munición, cooldowns y demás datos permanecen intactos; simplemente no se
emiten acciones de IA durante ese frame.

## Uso directo y ejecución incremental

La API original permanece accesible incluyendo:

```c
#include "geder_truth_gate.h"
```

Por tanto, otros sistemas pueden crear sus propios `GEDER_Pipeline` y usar
`GEDER_RunStep()` para repartir una puerta por frame. El bridge de enemigos usa
`GEDER_PipelineEvaluate()` porque sus cinco comprobaciones máximas son pequeñas
y deterministas.

## Compilación y pruebas

```bash
make test-geder-vendor
make test-truth-gate
make syntax-check
make audit
```

`test-geder-vendor` ejecuta las pruebas originales del paquete. El test del
bridge verifica aceptación retro, rechazo por distancia, acumulación de cuatro
verdades y rechazo/aceptación por línea de visión.

## Restricciones preservadas

- ISO C89.
- Sin `malloc`, `calloc`, `realloc` ni `free`.
- Sin `float` ni `double` en GEDER o en el bridge.
- Almacenamiento fijo dentro de cada enemigo.
- Puntaje Q16.16.
- Biblioteca original CC0 conservada con su licencia.
