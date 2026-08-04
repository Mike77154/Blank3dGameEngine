# conf_total (C89) — Config parser unificado (TOML + YAML + INI) sin malloc

**Objetivo:** Parsear **texto plano** (en memoria) escrito en **TOML** o **YAML** (y un **INI pragmático**) para obtener un **modelo canónico** (árbol de tablas/arrays/valores) que puedas consultar por *paths* tipo:

- `video.fullscreen`
- `db.ports[0]`
- `servers[1].host`

✅ **C89 puro**  
✅ **sin `stdio.h` / sin lectura de archivos** (tú pasas `char*` + len)  
✅ **sin `stdlib.h` / sin `malloc`** (arena propia)  
✅ **sin `string.h`** (comparaciones/copies propios)  
✅ **sin tipos C `float`/`double`**: números decimales en fixed-point 16.16 (`conf_fixed_t`)  

---

## 1) Qué soporta hoy

### TOML (frontend principal)
- Tables: `[a.b]`
- Array of tables: `[[a.b]]`
- Keys: bare + quoted + dotted keys
- Values:
  - strings: basic `"..."` (escapes + `\u`/`\U`), literal `'...'`
  - multiline strings: `\"\"\"...\"\"\"`, `'''...'''` (básico soportado)
  - bool: `true/false`
  - int: decimal + `0x`/`0o`/`0b`, con `_`
  - fixed-point: decimal con `.` y `e/E`, con `_` → `CONF_FIXED` 16.16. `inf/nan` no se aceptan en modo hard fixed.
  - datetime: se guarda como `CONF_DATETIME` (slice) con heurística (no se descompone en campos)
  - arrays: `[ ... ]` (con saltos de línea)
  - inline tables: `{ a = 1, b = 2 }`

> Nota: el 100% del TOML “formal” tiene rincones con reglas muy estrictas (reapertura de tablas, duplicados, etc.).  
> Esta lib es **muy usable** para configs reales, y además tiene `CONF_LOAD_OVERRIDE` para permitir overlays.

### YAML (subset orientado a configuración)
- block mapping:
  ```yaml
  a: 1
  b: true
  c:
    nested: hello
  ```
- block sequence:
  ```yaml
  list:
    - 1
    - 2
  ```
- quotes: `'single'` y `"double"` (double con escapes básicos + unicode por reutilización del unescape TOML)
- comments `#`

No soporta (a propósito): anchors/aliases, tags, flow collections, multi-doc, keys complejas.

### INI (pragmático)
- `[section]`
- `k=v` o `k: v`
- comments `;` y `#`
- valores se intentan tipar como bool/int/fixed/datetime (si no, string)

---

## 2) Modelo canónico (lo importante)

Valores:

- `CONF_TABLE`: mapa `key -> value` (linked list)
- `CONF_ARRAY`: lista de elementos (linked list)
- `CONF_BOOL`, `CONF_INT`, `CONF_FIXED`, `CONF_STRING`, `CONF_DATETIME`, `CONF_NULL`

Esto hace que **el engine no dependa del formato**.

---

## 3) Uso básico

### Inicializar (arena propia)

```c
#include "conf_total.h"

static unsigned char arena[64 * 1024];

conf_ctx_t cfg;
conf_ctx_init(&cfg, arena, (unsigned int)sizeof(arena));
```

### Cargar TOML (o auto-detect)

```c
conf_err_t e = conf_load_toml(&cfg, text, text_len);
/* o */
conf_err_t e2 = conf_load_auto(&cfg, text, text_len);
```

### Consultar

```c
int fs = conf_get_bool(&cfg, "video.fullscreen", 0);
long w = conf_get_int(&cfg, "video.width", 1280);
conf_fixed_t scale = conf_get_fixed(&cfg, "video.scale", CONF_FIXED_ONE);

conf_slice_t def = {0,0};
conf_slice_t q = conf_get_string(&cfg, "video.quality", def);
```

---

## 4) Defaults + overrides (capas)

La lib soporta una **cadena de fallback**:

- `defaults` (base)
- `user_cfg` (override)
- `cli_cfg` (override final)

```c
conf_ctx_set_fallback(&user_cfg, &defaults);
conf_ctx_set_fallback(&cli_cfg, &user_cfg);
```

Consulta:
- primero busca en `cli_cfg`
- si falta, cae a `user_cfg`
- si falta, cae a `defaults`

### Overrides por código

```c
conf_override_bool(&cli_cfg, "video.fullscreen", 1);
conf_override_int(&cli_cfg, "video.width", 1920);
conf_override_string(&cli_cfg, "video.quality", "ultra", 5);
conf_override_fixed(&cli_cfg, "video.scale", CONF_FIXED_FROM_INT(2));
```

### Override por scalar TOML (útil para CLI)

```c
conf_override_scalar_toml(&cli_cfg, "video.width", "1920", 4);
conf_override_scalar_toml(&cli_cfg, "video.quality", "\"ultra\"", 7);
```

---

## 5) Flags de carga

```c
conf_ctx_set_load_flags(&cfg, CONF_LOAD_COPY_SLICES | CONF_LOAD_OVERRIDE);
```

- `CONF_LOAD_COPY_SLICES`: copia keys/strings al arena (recomendado)
- `CONF_LOAD_OVERRIDE`: permite que una asignación posterior reemplace una anterior

---

## 6) Compilación

Ejemplo:

```bash
cc -std=c89 -Wall -Wextra -pedantic -c conf_total.c
```

---

## 7) Roadmap (si quieres endurecerlo a “spec-hard”)

- Validación TOML estricta completa (reapertura de tablas, prohibiciones, tipos homogéneos en arrays, etc.)
- YAML más completo (flow collections, anchors/aliases)
- Mejor parser RFC3339 para datetimes (sin libc, en struct propio)
- Hash map opcional (sin malloc, con tabla fija) para lookup O(1)


## Patch hard-fixed aplicado

- TOML: el lexer ahora clasifica números antes que bare keys, así `width=1280` entra como número.
- TOML: las keys numéricas quedan permitidas como key parts para no romper casos TOML válidos.
- YAML: el lookahead `scalar:` ahora usa buffer de 2 tokens; ya no se come el `:`.
- Numerales decimales: convertidos a `CONF_FIXED` 16.16, sin tipos C `float` ni `double`.
- API decimal nueva: `conf_get_fixed()` / `conf_override_fixed()`.
- Se añadió `tests/test_conf_total.c` con humo TOML/YAML/INI/overrides.
