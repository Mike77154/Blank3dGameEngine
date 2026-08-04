# DDSL2 0.3.1 — parche de seguridad semántica

## Correcciones

- Una acción adicional sin coma ya no puede escapar de un `if`; ahora es error de sintaxis.
- Las sentencias de expresión rechazan tokens sobrantes en la misma línea.
- Los números con varios puntos, demasiado largos o fuera del rango fixed-point se rechazan.
- La VM vuelve a compilar bajo C89 estricto sin declaraciones mezcladas con instrucciones.
- El backpatching de `if/elif/else` ya no usa un array fijo de 64 saltos.
- Runtime y VM propagan errores cuando el store está lleno o una escritura falla.
- El bytecode copia a la arena sus strings e identificadores y deja de depender del buffer fuente.
- El transpilador representa bytes no ASCII con escapes octales de tres dígitos, evitando escapes hexadecimales ambiguos.
- El Makefile crea `build/` automáticamente.

## Pruebas de regresión

`make check` verifica:

- rechazo de comas ausentes;
- rechazo de números inválidos;
- paridad runtime/VM con 80 ramas;
- independencia del bytecode respecto al source original;
- propagación de errores del store;
- escapes UTF-8 seguros;
- compilación C89 del C transpilado;
- ejecución real del C transpilado.
