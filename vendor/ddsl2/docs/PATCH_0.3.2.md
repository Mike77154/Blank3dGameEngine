# DDSL2 0.3.2 — validación semántica y API pública

## Correcciones

- `ddsl_api_version_major()` y `ddsl_api_version_minor()` ya están declaradas en `API/ddsl_api.h`.
- Se añadieron macros de versión de paquete: `DDSL_VERSION_MAJOR`, `DDSL_VERSION_MINOR` y `DDSL_VERSION_PATCH`.
- `ddsl_semantics_check_program()` dejó de ser una comprobación nula y ahora valida:
  - coherencia de `first`/`last` y listas del AST;
  - tipos de sentencia, acción, expresión y operador;
  - hijos obligatorios y forma válida de `if/elif/else`;
  - profundidad y cantidad máxima de nodos para detectar ciclos o AST hostiles;
  - forma y longitud de identificadores;
  - longitud de strings frente al store fijo;
  - prohibición de asignar a los literales reservados `true` y `false`.
- El constructor de IR y el runtime validan el AST antes de recorrerlo.
- Los nombres y strings demasiado largos ahora producen error, en lugar de colisionar o truncarse silenciosamente.

## Compatibilidad

El lenguaje sigue siendo dinámico: los identificadores inexistentes pueden actuar como strings, las conversiones numéricas conservan su comportamiento y la división entre cero sigue devolviendo cero según el runtime existente.

El único endurecimiento visible es que scripts que dependían del truncado silencioso de claves o valores ahora se rechazan con un error semántico.

## Configuración nueva

```c
#define DDSL_SEMANTICS_MAX_DEPTH 256
#define DDSL_SEMANTICS_MAX_NODES 16384
```

Ambos límites pueden redefinirse antes de incluir las cabeceras.

## Pruebas añadidas

- compilación de un consumidor de la API de versión;
- coincidencia entre funciones de versión y `config.h`;
- rechazo de asignaciones a `true`/`false`;
- rechazo de claves que exceden `DDSL_MAX_KEY_LEN`;
- rechazo de strings que exceden `DDSL_MAX_VALUE_LEN`;
- rechazo de nodos AST con tipos desconocidos.
