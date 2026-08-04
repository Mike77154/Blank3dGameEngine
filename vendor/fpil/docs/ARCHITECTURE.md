# Arquitectura de FPIL v18

## Mapa de módulos

| Módulo solicitado | Implementación | Responsabilidad |
|---|---|---|
| API | `fpi_api.*`, `fpi.h` | Contexto, carga, bindings, ejecución y compilación. |
| CLI | `tools/fpi_cli.c` | Inspector, transpiler, runner y VM. |
| IR | `fpi_ir.*` | Reglas y términos intermedios construidos en arena. |
| VM | `fpi_vm.*` | Ejecución validada secuencial o two-phase. |
| alloc | `fpi_alloc.*` | Operaciones acotadas de memoria, sin heap. |
| arena | `fpi_arena.*` | Allocator lineal sobre buffer del host. |
| ast / AST | `fpi_ast.*` | Reglas, términos y pool compacto de RHS. |
| bytecode | `fpi_bytecode.*` | Desensamblado del programa. |
| common | `fpi_common.h` | Macros base. |
| compiler | `fpi_compiler.*` | AST → IR → programa. |
| config | `fpi_config.h` | Capacidades y política Q16.16. |
| error | `fpi_error.*` | Resultados, mensajes y spans. |
| io | `fpi_io.*` | Lectura acotada sin truncamiento. |
| lexer | `fpi_lexer.*` | Tokenización estructural. |
| token | `fpi_token.*` | Tipos de token. |
| parser | `fpi_parser.*` | DSL → AST. |
| opcodes | `fpi_opcodes.*` | Instrucciones de VM. |
| program | `fpi_program.*` | Código, constantes, rangos y validación. |
| registry | `fpi_registry.*` | IDs persistentes, aliases y callbacks. |
| runtime | `fpi_runtime.*` | Intérprete directo. |
| semantics | `fpi_semantics.*` | Validación posterior al parse. |
| builtins | `fpi_builtins/` | Estado configurable y condiciones de verdad. |
| span | `fpi_span.*` | Offset, longitud, línea y columna. |
| store | `fpi_store.*` | Valores fixed publicados por el host. |
| stream | `fpi_stream.*` | Cursor de texto con CRLF y posición. |
| symtab | `fpi_symtab.*` | Compatibilidad nominal sobre registry. |
| polysym | `fpi_polysym.*` | Texto simbólico → Q16.16. |
| transpiler | `fpi_transpiler.*` | FPIL normalizado y JSON. |
| types | `fpi_types.h` | Tipos y modos de ejecución. |
| util | `fpi_util.*` | ASCII, strings y hashing. |
| value | `fpi_value.*` | Parseo, formato y matemática Q16.16. |
| warper | `fpi_warper.*` | Fachada no-heap solicitada con VM interna. |
| wrapper | `fpi_wrapper.h` | Alias ortográfico de `warper`. |

## Contexto y doble banco

```text
FPI_Context
├── registries[2]  ── IDs, aliases, callbacks, nombres
├── ast_banks[2]   ── reglas, términos, pool de RHS
├── polysym         ── símbolos de valor persistentes
├── store           ── valores publicados por el host
├── last_error
├── run_options
├── generation
└── active_bank
```

La copia completa del registry activo al staging garantiza que una recarga conserva IDs y bindings. Los símbolos nuevos se anexan al final.

## AST compacto

```text
FPI_Rule
├── condition_first / condition_count
└── action_first    / action_count

FPI_Term
├── symbol_id
├── FPI_ValueRef ─────────────► value_pool
└── span
```

No hay un arreglo de texto grande incrustado en cada término. Los RHS ocupan solamente su longitud real dentro del pool.

## IR y programa

El IR se construye dentro de una arena externa. El compilador calcula la capacidad exacta para código, constantes, tabla de reglas y copias de cadenas.

```text
AST
 │
 ▼
IRRule + IRTerm
 │
 ▼
FPI_Program
├── instructions
├── constants
├── rule ranges
└── source_generation
```

Secuencia típica:

```text
EVAL_COND
JUMP_IF_FALSE
...
EXEC_ACT
...
RULE_FIRED
HALT
```

`fpi_program_validate()` comprueba opcodes, índices de constantes, saltos, rangos de regla y HALT final.

## Frontera agnóstica

```text
FPIL core                          Host
─────────                          ────
parsea nombres          ◄──────── vocabulario elegido
entrega symbol_id       ────────► callbacks concretos
resuelve fixed/polysym  ◄──────── datos y equivalencias
ordena reglas           ────────► efectos del dominio
```

El core no contiene movimiento, gráficos, red, UI ni modelos propios de una aplicación.
