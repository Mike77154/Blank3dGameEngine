# Migration from ddsl2_lib

## Renombres intencionales

- `ddsl_tokens_free()` → `ddsl_tokens_reset()`
- `ddsl_ast_free_expr()` → `ddsl_ast_discard_expr()`
- `ddsl_ast_free_actions()` → `ddsl_ast_discard_actions()`
- `ddsl_ast_free_clauses()` → `ddsl_ast_discard_clauses()`
- `ddsl_ast_free_program()` → `ddsl_ast_discard_program()`

## Números

- Antes: API numérica basada en `double`.
- Ahora: API numérica basada en `ddsl_fixed`.

```c
ddsl_fixed x;
ddsl_fixed_parse_cstr("12.5", &x);
ddsl_store_set_num(&store, "volume", x);
```

## Includes

- Antes: `#include "ddsl/ddsl.h"`
- Ahora: `#include "API/ddsl_api.h"` o `#include "common/ddsl.h"`
