# Patch Notes - ultra89 pass

## Cambios principales

- `evcore_subscribe()` ahora devuelve un handle estable (`evcore_sub_id`) en vez de `0/-1`.
- `evcore_emit()` usa snapshot local para evitar bugs cuando se registran o desregistran listeners durante el dispatch.
- `condcore` y `actioncore` ya tienen `unregister()`.
- `actioncore_exec()` ahora devuelve `1/0` para saber si realmente ejecutó.
- Se agregó `evact` con tabla de reglas `evento -> condición -> acción`.
- Se agregaron `reset()` por subsistema.
- El bridge externo ahora es opcional en CMake y no rompe el build standalone.
- Se agregó self-test C89.

## Semántica útil

- Nuevos listeners registrados durante un `emit` **no** reciben ese evento actual.
- Listeners quitados antes de su turno **no** se ejecutan.
- Reutilizar el mismo slot en el mismo frame no dispara callbacks equivocados.
- `evact_attach_all()` conecta el motor de reglas al bus de eventos sin duplicarlo.

## Build

### Standalone

```bash
cmake -S . -B build -DCONDOR_EVACT_BUILD_TESTS=ON -DCONDOR_EVACT_WITH_INPUT_BRIDGE=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

### Con bridge externo

Requiere:

- `../input_scanner/input_ev_handler.h`
- target CMake `input_scanner`

```bash
cmake -S . -B build -DCONDOR_EVACT_WITH_INPUT_BRIDGE=ON
```
