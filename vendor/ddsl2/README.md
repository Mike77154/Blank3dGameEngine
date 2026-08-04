# dsl_minimum C89 fixed-point edition

Versión parcheada: **0.3.2**. Incluye validación estricta de sintaxis y números, backpatching sin límite fijo de 64 ramas, bytecode dueño de sus literales y validación semántica previa a IR/runtime.

Conversión expansiva de `ddsl2_lib` a una base modular C89, con memoria fija entregada por el caller y números en fixed-point.

## Qué cambió

- **C89 estricto**: probado con `gcc -std=c89 -pedantic -Wall -Wextra -Werror`.
- **Sin asignación dinámica**: lexer, parser, AST, IR y bytecode usan buffers fijos o arena externa.
- **Sin `double` / `float`**: los números usan `ddsl_fixed`, por defecto escala decimal `1000`.
- **Parser numérico manual**: se retiró `strtod`.
- **Salida numérica manual**: se retiró formateo `%g`.
- **Árbol expandido**: módulos separados para token, lexer, parser, AST, IR, bytecode, VM, runtime, API, stdlib propia, registry, opcodes, etc.
- **Semántica defensiva**: rechaza AST inválidos, asignaciones a `true`/`false` y datos que serían truncados por los buffers fijos.

## Build rápido

```sh
cd dsl_minimum
make check
```

El `Makefile` crea automáticamente el directorio `build/`; la compilación se ejecuta desde la raíz de `dsl_minimum`.

## CLI

```sh
./build/ddsl_cli run examples/scripts/demo.ddsl --dump
./build/ddsl_cli tokens examples/scripts/demo.ddsl
./build/ddsl_cli ir examples/scripts/demo.ddsl
./build/ddsl_cli bc examples/scripts/demo.ddsl
./build/ddsl_cli transpile examples/scripts/demo.ddsl
```

## API principal

Incluye todo desde:

```c
#include "API/ddsl_api.h"
```

La cabecera pública declara también:

```c
int ddsl_api_version_major(void);
int ddsl_api_version_minor(void);
```

o la cabecera núcleo:

```c
#include "common/ddsl.h"
```

## Fixed point

`ddsl_fixed` vive en `types/fixed.h`.

Configuración por defecto:

```c
#define DDSL_FIXED_SCALE 1000L
#define DDSL_FIXED_FRAC_DIGITS 3
```

Ejemplos internos:

- `1` se guarda como `1000`.
- `0.8` se guarda como `800`.
- `3.125` se guarda como `3125`.

## Estructura

```text
dsl_minimum/
├─ token/
├─ symtab/
├─ polysym/
├─ config/
├─ lexer/
├─ parser/
├─ ast/
├─ bytecode/
├─ VM/
├─ runtime/
├─ CLI/
├─ IR/
├─ Transpiler/
├─ arena/
├─ alloc/
├─ program/
├─ warper/
├─ types/
├─ registry/
├─ common/
├─ build/
├─ API/
├─ stdlib/
├─ compiler/
├─ span/
├─ util/
├─ io/
├─ error/
├─ stream/
├─ value/
├─ store/
├─ semantics/
├─ opcodes/
└─ built-ins/
```

## Nota de migración

Los helpers de descarte no liberan memoria individual; el ciclo correcto es rebobinar o reiniciar la arena externa. En esta edición se prefiere `ddsl_tokens_reset()` y `ddsl_ast_discard_*()` para que el contrato sea explícito.

Desde 0.3.2, identificadores de `DDSL_MAX_KEY_LEN` bytes o más y strings de `DDSL_MAX_VALUE_LEN` bytes o más se rechazan en vez de truncarse. Consulta `docs/PATCH_0.3.2.md`.

## Licencia

la licencia la puedes consultar en el archivo Creative Commons, ya  que es CC0 tal como el file "Licence"
