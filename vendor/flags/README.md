# flags_c89 (C89, sin stdint, sin float, sin malloc, sin stdio)

Implementación en C89 inspirada en tus módulos Python:

- `flags_helpers.py` → `flags_state.*` (parser, estado, apply_adds)
- `flags_loader.py`  → `flags_loader.*` (load_once)
- `flagstore.py`     → `flagstore.*` (KV store en memoria)

## ✅ Restricciones

- **C89** (sin `//`, sin declaraciones después de statements)
- **Sin `stdint.h`**
- **Sin `float/double`** → usa **fixed-point Q16.16** (`flags_fx_t` = `long`)
- **Sin `malloc/free`** → todo es memoria del caller (arrays/buffers)
- **Sin `stdio.h`** → sin `fopen/printf`; IO se hace con callbacks (`FlagsIO`)

## Formatos soportados

### 1) INI-like (línea por línea)

```ini
# comentario
music.enabled = true
affection.value = 0.25
affection.value += 0.05
```

- `key=value` se guarda en `FlagStore` (directo)
- `key+=delta` se guarda como `key__add__` y se aplica sobre `mem` (si existe)

### 2) JSON (objeto)

```json
{
  "music": { "enabled": true },
  "affection": { "value": 0.25 }
}
```

Se aplana a claves con puntos: `music.enabled`, `affection.value`.

## Uso rápido (sin threads)

1. Inicializa dos stores:
   - `flagstore`: para asignaciones directas
   - `mem`: para aplicar deltas "+=" (puede ser el mismo si quieres)

2. Crea el loader y llama `flags_loader_load_once()` en tu loop.

### Ejemplo (pseudo-código)

> **Nota:** sin `stdio`. Implementa tu IO (leer archivo y mtime) con `FlagsIO`.

```c
#include "flags_loader.h"
#include "flags_io_posix.h" /* opcional si activas FLAGS_ENABLE_POSIX_IO */

#define STORE_CAP 128
#define STORE_POOL 4096
#define STATE_ITEMS 256
#define STATE_POOL 4096
#define FILE_BUF 8192

static FlagStoreEntry g_store_entries[STORE_CAP];
static char g_store_pool[STORE_POOL];

static FlagStoreEntry g_mem_entries[STORE_CAP];
static char g_mem_pool[STORE_POOL];

static FlagsKV g_state_items[STATE_ITEMS];
static char g_state_pool[STATE_POOL];

static char g_file_buf[FILE_BUF];

int main(void)
{
    FlagStore store;
    FlagStore mem;
    FlagsLoader loader;
    FlagsIO io;

    flagstore_init(&store, g_store_entries, STORE_CAP, g_store_pool, STORE_POOL);
    flagstore_init(&mem,   g_mem_entries,   STORE_CAP, g_mem_pool,   STORE_POOL);

#if FLAGS_ENABLE_POSIX_IO
    io = flags_io_posix();
#else
    /* io.get_mtime / io.read_all => tus callbacks */
#endif

    flags_loader_init(&loader,
                      "./monika.flags",
                      &store,
                      &mem,
                      "affection.",          /* prefix_for_adds (opcional) */
                      1, FLAGS_FX_FROM_INT(0), /* clamp lo */
                      1, FLAGS_FX_FROM_INT(1), /* clamp hi */
                      0, 0,                  /* target_map */
                      g_state_items, STATE_ITEMS,
                      g_state_pool, STATE_POOL);

    /* loop */
    for (;;) {
        (void)flags_loader_load_once(&loader, &io, g_file_buf, FILE_BUF, 0, 0);
        /* ... tu lógica ... */
    }
}
```

## Ajustes

- Cambia tamaños en `flags_config.h`.
- Si necesitas POSIX IO:
  - en `flags_config.h`: `#define FLAGS_ENABLE_POSIX_IO 1`
  - compila `flags_io_posix.c`

## Compilación (ejemplo GCC)

```bash
gcc -std=c89 -Wall -Wextra -pedantic \
  flags_fx.c flags_pool.c flags_value.c flagstore.c flags_state.c flags_loader.c flags_util.c \
  -o flags_demo
```

(Agrega `flags_io_posix.c` si lo habilitas.)
