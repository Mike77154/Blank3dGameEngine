# Política sin heap

FPIL no llama a asignadores dinámicos. La memoria procede de:

1. Estructuras estáticas poseídas por el host.
2. Pools fijos dentro del contexto.
3. Arenas inicializadas sobre buffers externos.
4. El pool estático opcional usado por `fpi_create()`.

## Contexto

```c
static FPI_Context context;
fpi_context_init(&context);
```

`fpi_create()` toma una ranura de `FPI_STATIC_CONTEXTS`; `fpi_destroy()` reinicia y libera esa ranura lógica, sin llamar a un liberador de heap.

## Bytecode

```c
static unsigned char memory[65536];
FPI_Arena arena;
FPI_Program program;

fpi_arena_init(&arena, memory, sizeof(memory));
fpi_compile(&context, &arena, &program);
```

La arena es lineal. Para recompilar, se reinicia o se vuelve a inicializar.

## Lectura de archivos

`fpi_context_load_buffered()` usa memoria entregada por el host. `fpi_context_load_file()` usa un buffer estático de conveniencia y no es la opción ideal para cargas simultáneas.

## Fallo por capacidad

Las capacidades están en `fpi_config.h`. Al agotarse se devuelve un código específico, por ejemplo:

```text
FPI_ERR_FILE_TOO_LARGE
FPI_ERR_TOO_MANY_RULES
FPI_ERR_TOO_MANY_TERMS
FPI_ERR_SYMBOL_TABLE_FULL
FPI_ERR_STRING_POOL_FULL
FPI_ERR_POLYSYM_FULL
FPI_ERR_ARENA_EXHAUSTED
```

## Auditoría

```sh
make audit
```

La regla inspecciona `.c` y `.h` para rechazar asignadores dinámicos, tipos reales nativos y funciones matemáticas reales prohibidas.
