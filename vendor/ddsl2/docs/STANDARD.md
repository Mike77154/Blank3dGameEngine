# dsl_minimum standard profile

Perfil: `dsl_minimum_c89_fixed`.

## Reglas de plataforma

1. Código C89 compatible.
2. Sin uso de memoria dinámica de la biblioteca estándar.
3. El usuario entrega almacenamiento fijo: arrays, store y arena.
4. No se usan tipos de coma flotante.
5. Todo número DSL se convierte a `ddsl_fixed`.
6. Los tamaños máximos se configuran por macros antes de incluir las cabeceras.

## Macros principales

```c
DDSL_MAX_TOKENS
DDSL_MAX_FLAGS
DDSL_MAX_KEY_LEN
DDSL_MAX_VALUE_LEN
DDSL_VM_STACK_MAX
DDSL_FIXED_SCALE
DDSL_FIXED_FRAC_DIGITS
DDSL_MAX_SYMBOLS
DDSL_MAX_POLYSYMS
DDSL_MAX_REGISTRY_ITEMS
DDSL_SEMANTICS_MAX_DEPTH
DDSL_SEMANTICS_MAX_NODES
```

## Flujo recomendado

```text
source
  │
  ▼
lexer/token
  │
  ▼
parser/span/error
  │
  ▼
ast/semantics
  │
  ▼
IR/compiler
  │
  ▼
bytecode/opcodes
  │
  ▼
VM/runtime/store/value
```

## Contrato de memoria

- `ddsl_arena_init(&arena, buffer, bytes)` recibe memoria externa.
- `ddsl_arena_mark()` y `ddsl_arena_rewind()` controlan temporales.
- El bytecode copia sus literales e identificadores a la arena; no conserva préstamos del buffer fuente.
- Los stores y vectores tienen capacidad fija.
- Ante capacidad insuficiente, las funciones devuelven error y llenan `ddsl_error` cuando aplica.
- La fase semántica rechaza claves y strings que no caben íntegramente en los buffers del store; no se permite depender de truncado silencioso.
