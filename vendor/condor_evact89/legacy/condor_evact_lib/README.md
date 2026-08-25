# condor_evact

Micro-librería C89 para **eventos + condiciones + acciones + reglas**, pensada para targets austeros:

- sin heap
- tablas estáticas
- callbacks simples
- C89 estricto
- portable a toolchains viejos

## Núcleos

- `evcore`: bus de eventos con dispatch seguro ante mutaciones.
- `condcore`: registro/evaluación de condiciones por handle estable.
- `actioncore`: registro/ejecución de acciones por handle estable.
- `evact`: tabla de reglas `evento -> condición -> acción`.
- `evcore_input_bridge`: puente opcional a `input_scanner`.

## Mejoras metidas en esta versión

- dispatch por snapshot: altas nuevas no reciben el evento actual
- bajas durante dispatch no se ejecutan si aún no les tocaba
- handles estables con serial por slot
- `unregister` en condiciones, acciones y reglas
- `subscribe_unique` y `rule_register_unique`
- `reset` por subsistema
- build standalone sin forzar el bridge externo
- self-test C89

## Flujo típico

```c
#include "evact.h"

static int is_code_7(const evcore_event_t *evt, void *user)
{
    (void)user;
    return evt != 0 && evt->code == 7;
}

static void fire_action(const evcore_event_t *evt, void *user)
{
    (void)evt;
    (void)user;
}

void setup(void)
{
    condcore_id c;
    actcore_id a;

    c = condcore_register(is_code_7, 0);
    a = actcore_register(fire_action, 0);

    evact_rule_register(1001, EVCORE_MATCH_ANY, c, a);
    evact_attach_all();
}
```

## Build standalone

```bash
cmake -S . -B build -DCONDOR_EVACT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build con bridge externo

El bridge está apagado por default. Para activarlo necesitas:

- `../input_scanner/input_ev_handler.h`
- target CMake `input_scanner`

```bash
cmake -S . -B build -DCONDOR_EVACT_WITH_INPUT_BRIDGE=ON
```
