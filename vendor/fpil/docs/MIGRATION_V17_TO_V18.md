# Migración de FPIL v17 a v18

## Contexto poseído por el host

Recomendado:

```c
static FPI_Context context;
fpi_context_init(&context);
```

Se conservan nombres de compatibilidad:

```c
fpi_init_context
fpi_load_script_text
fpi_load_script_buffered
fpi_load_script
```

Los wrappers antiguos de carga devuelven `0/-1`; la API nueva devuelve el código `FPI_Result` exacto.

## Números

Toda la stdlib numérica usa `FPI_Fixed` Q16.16:

```c
FPI_Fixed value;
fpi_fixed_parse("12.5", &value);
```

No hay tipos reales nativos ni dependencia matemática externa.

## Callbacks

```c
int condition(void* bind_user,
              void* run_user,
              int symbol_id,
              const FPI_Value* value);

void action(void* bind_user,
            void* run_user,
            int symbol_id,
            const FPI_Value* value);
```

`bind_user` dura con el registro; `run_user` puede cambiar en cada tick.

## Registro y reload

En v18:

- el orden de aparición no renumera símbolos existentes;
- los callbacks sobreviven al reload;
- una carga fallida no sustituye el AST bueno;
- el bytecode viejo queda invalidado por generación;
- ejecutar un símbolo sin callback devuelve error explícito.

Después de una recarga exitosa, recompila el `FPI_Program` usado por la VM.

## Estado configurable

Default compatible:

```text
:state=0:state=10
```

Nombre alternativo:

```c
fpi_state_words_from_base(&words, "mode");
```

Valor simbólico:

```c
fpi_define_value_symbol(&context, "idle", 0L);
fpi_define_value_symbol(&context, "running", FPI_FIXED_ONE);
```

Resultado:

```text
:mode=idle:mode=running
```

## Archivos

Preferible para control y reentrancia:

```c
static char buffer[65536];
int rc = fpi_context_load_buffered(&context,
                                   "script.fpi",
                                   buffer,
                                   sizeof(buffer));
```

Si no cabe, devuelve `FPI_ERR_FILE_TOO_LARGE`. No se carga un prefijo parcial.

## Two-phase

El límite oculto de 64 coincidencias desapareció. Intérprete y VM usan un bitset dimensionado por `FPI_MAX_RULES`.

## Separación de paquetes

- `libfpil_core.a`: protocolo y ejecución agnósticos.
- `libfpil_builtins.a`: estado configurable y verdad, opcionales.
- `libfpil_stdlib.a`: variables, temporizadores y activación genéricos, opcionales.

Los verbos de un dominio concreto deben registrarse desde un paquete externo.
