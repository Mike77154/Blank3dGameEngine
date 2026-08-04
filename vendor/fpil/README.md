# FPIL v18 — runtime de reglas agnóstico en C89

FPIL procesa un DSL de reglas con la forma:

```text
:condicion=valor,otra_condicion:accion=valor,otra_accion
```

El núcleo **no sabe qué significa** una condición o una acción. El programa anfitrión registra el vocabulario y conecta callbacks. Por eso puede usarse para workflows, automatización, simuladores, documentos interactivos, control de procesos, interfaces, agentes o motores sin quedar amarrado a ninguno.

## Garantías de la fase de estabilización

- C89 estricto.
- Sin `malloc`, `calloc`, `realloc` ni `free`.
- Sin `float` ni `double`.
- Aritmética Q16.16 con saturación.
- Registro persistente: los IDs y callbacks sobreviven al reload.
- Reload transaccional: un script inválido no destruye el activo.
- AST compacto con términos y textos en pools.
- Errores con código, línea y columna; los límites no truncan silenciosamente.
- Intérprete, IR, compilador, bytecode y VM.
- Ejecución secuencial inmediata o en dos fases.
- Valores simbólicos (`polysym`) configurados por el host.
- Memoria estática y arenas entregadas por el host.

## `state=0` es el default, no una palabra reservada

La palabra de protocolo y el valor son configurables por separado:

```text
:state=0:state=1
:state=idle:state=running
:mode=0:mode=1
:mode=idle:mode=running
```

El host decide que `idle` vale `0`, que `running` vale `1` y que la palabra base será `mode`:

```c
fpi_define_value_symbol(&context, "idle", 0L);
fpi_define_value_symbol(&context, "running", FPI_FIXED_ONE);
fpi_state_words_from_base(&words, "mode");
fpi_state_builtin_init(&state_builtin, &context,
                       &mode, &transitioned, &words);
```

También puede llenar `FPI_StateWords` manualmente y usar cinco nombres completamente distintos.

## Compilación

```sh
make
make strict
make test
make audit
make example
```

Salidas de build:

```text
libfpil_core.a
libfpil_builtins.a
libfpil_stdlib.a
fpi_cli
fpi_tests
fpi_example_host
```

## CLI

```sh
./fpi_cli \
  --define idle=0 \
  --define running=1 \
  --state-word mode \
  --state idle \
  --run 3 \
  examples/state_symbols.fpi
```

Opciones principales:

```text
--tokens       inspección léxica; imprime RHS completos como RHS_VALUE
--ast          resumen del AST compacto
--bytecode     compilación y desensamblado
--transpile    FPIL normalizado
--json         representación JSON
--run N        ejecutar N ticks
--vm           usar la VM
--two-phase    evaluar todas las reglas antes de ejecutar acciones
--stop-first   detenerse en la primera coincidencia
--state-word   sustituir la palabra base `state`
--state        estado inicial numérico o simbólico
--define       definir NAME=VALUE en polysym
```

## Integración mínima

Véase [`examples/custom_state_host.c`](examples/custom_state_host.c).

La forma recomendada sin heap es que el host posea el contexto:

```c
static FPI_Context context;
fpi_context_init(&context);
```

`fpi_create()` también es sin heap: toma una entrada de un pool estático cuyo tamaño se configura con `FPI_STATIC_CONTEXTS`.

Para la VM, el host proporciona el buffer:

```c
static unsigned char program_memory[65536];
FPI_Arena arena;
FPI_Program program;

fpi_arena_init(&arena, program_memory, sizeof(program_memory));
fpi_compile(&context, &arena, &program);
```

## Capas

```text
Host
 ├─ vocabulario y callbacks propios
 ├─ builtins genéricos opcionales
 └─ stdlib genérica opcional
            │
            ▼
API / registry / polysym / store
            │
            ▼
stream → lexer → parser → AST → semantics → IR → compiler → bytecode → VM
                                └───────────────────────────────→ runtime
```

## Documentación

- [`docs/DSL_PROTOCOL.md`](docs/DSL_PROTOCOL.md)
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- [`docs/MIGRATION_V17_TO_V18.md`](docs/MIGRATION_V17_TO_V18.md)
- [`docs/NO_HEAP_POLICY.md`](docs/NO_HEAP_POLICY.md)
- [`BUILD_REPORT.txt`](BUILD_REPORT.txt)

## Licencia

CC0-1.0. Véase `LICENSE`.
