# conf_total hard-fixed patch

## Cambios aplicados

- Reemplazado el valor decimal de `CONF_FLOAT` por `CONF_FIXED`.
- Añadido `conf_fixed_t` 16.16 en `conf_total.h`.
- API decimal pública nueva:
  - `conf_get_fixed()`
  - `conf_override_fixed()`
- Eliminados los tipos C `float` y `double` del código de la librería.
- TOML lexer: los números se tokenizan antes que bare keys, arreglando casos como `width=1280`.
- TOML key parser: acepta `TT_NUMBER` como parte de key para conservar bare keys numéricas.
- YAML parser: el lookahead `scalar:` ahora usa buffer de 2 tokens y ya no consume el `:`.
- Override API: ahora reemplaza valores existentes aunque `CONF_LOAD_OVERRIDE` no esté activado.
- Añadido `tests/test_conf_total.c` con pruebas de humo para TOML, YAML, INI, fixed-point y overrides.

## Nota de compatibilidad

`inf`/`nan` ya no se aceptan como numerales decimales porque no tienen representación segura en fixed-point 16.16.

## Comando probado

```sh
gcc -std=c89 -Wall -Wextra -pedantic conf_total.c tests/test_conf_total.c -o test_conf_total
./test_conf_total
```

Resultado esperado:

```txt
all conf_total tests passed
```
