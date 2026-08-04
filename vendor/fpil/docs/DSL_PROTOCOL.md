# Protocolo DSL de FPIL v18

## Gramática práctica

Cada regla ocupa una línea:

```text
:condicion[,condicion...]:accion[,accion...]
```

Cada término puede llevar un RHS:

```text
:nombre=valor:nombre=valor
```

Los nombres no tienen semántica reservada en el core. Comienzan con letra o `_`; después aceptan letras, dígitos y `_`. Se normalizan a minúsculas.

Una línea no vacía que no empiece con `:` devuelve `FPI_ERR_SYNTAX`; ya no se ignora como basura.

## Comentarios y espacios

`;` inicia un comentario hasta el final de la línea:

```text
; comentario completo
:ready:emit="ok" ; comentario final
```

## Valores

### Sin comillas

Terminan en coma, dos puntos, comentario o fin de línea:

```text
:threshold=12.5:set=result
```

Los espacios finales se eliminan.

### Entre comillas

Pueden contener comas y dos puntos:

```text
:ready:emit="hello, world: ok"
```

Se aceptan comillas simples o dobles. Escapes:

```text
\n  nueva línea
\r  retorno de carro
\t  tabulación
\\  barra invertida
\"  comilla doble
\'  comilla simple
\,  coma
\:  dos puntos
```

Un escape inválido o una cadena sin cerrar devuelve un error explícito.

### Q16.16

Las formas numéricas válidas se almacenan como `FPI_Fixed` Q16.16. El rango semántico es aproximadamente:

```text
-32768.0 ... 32767.99998
```

Las operaciones de la stdlib saturan en los extremos. No se usa punto flotante nativo.

### Polysym

El host puede mapear texto a fixed:

```c
fpi_define_value_symbol(&context, "idle", 0L);
fpi_define_value_symbol(&context, "running", FPI_FIXED_ONE);
```

`idle` y `running` son datos configurables, no palabras del parser.

### Store

El host puede publicar valores fixed persistentes:

```c
fpi_store_value(&context, "limit", fpi_fixed_from_int(10L));
```

Los resolvers aceptan `limit` y `%limit`.

## Protocolo de estado opcional

El paquete `fpi_builtins` aporta una convención. Con la base `state` registra:

```text
Condiciones: state, stategreater, statelesser
Acciones:    state, incstate
```

Con base `mode`:

```text
Condiciones: mode, modegreater, modelesser
Acciones:    mode, incmode
```

```c
FPI_StateWords words;
fpi_state_words_from_base(&words, "mode");
```

El núcleo sigue siendo agnóstico: puede representar una fase industrial, estado de red, capítulo, paso de workflow o cualquier otro dominio.

## Registro y aliases

```c
fpi_bind_cond(&context, "ready", ready_callback, data);
fpi_bind_act(&context, "emit", emit_callback, data);

fpi_alias_cond(&context, "prepared", "ready");
fpi_alias_act(&context, "send", "emit");
```

Condiciones y acciones tienen namespaces separados. Los IDs existentes no cambian aunque el orden textual del script cambie.

Un símbolo ejecutado sin callback devuelve `FPI_ERR_UNBOUND_SYMBOL` tanto en intérprete como en VM.

## Modos de ejecución

### Secuencial inmediata

Cada regla coincidente ejecuta sus acciones antes de evaluar la siguiente:

```c
options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
```

### Dos fases

Primero se marcan todas las reglas coincidentes y después se ejecutan sus acciones:

```c
options.exec_mode = FPI_EXEC_TWO_PHASE;
```

El bitset se dimensiona con `FPI_MAX_RULES`; no existe el antiguo corte de 64 reglas.

En ambos modos:

```c
options.stop_on_first_match = 1;
```

## Reload transaccional

`fpi_context_load_text()` trabaja sobre el banco inactivo:

```text
copiar registry activo → limpiar AST staging → parsear → validar
                                  │
                     error ───────┴────── éxito
                     conservar            intercambiar bancos
                     generación           incrementar generación
```

En una carga fallida se conservan AST, IDs, callbacks y generación activos.

Todo `FPI_Program` guarda la generación de origen. La VM rechaza bytecode viejo con `FPI_ERR_GENERATION_MISMATCH`.

## Capacidades predeterminadas

```text
Reglas                         256
Términos totales               16384
Términos por lado de regla     32
Símbolos de condición          512
Símbolos de acción             512
Identificador                  63 caracteres
Pool de valores por AST        131072 bytes
Archivo de conveniencia        262144 bytes
Polysyms                       256
Store                          256 entradas
```

Un RHS individual usa longitud de 16 bits y se rechaza al superar 65535 bytes; normalmente el pool del AST se agotará antes. Todo exceso devuelve error: nunca se carga un prefijo truncado.
