# Audit notes

Comandos usados durante la conversión:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -I. */*.c -o ddsl_cli
./ddsl_cli run examples/scripts/demo.ddsl --dump
./ddsl_cli ir examples/scripts/demo.ddsl
./ddsl_cli bc examples/scripts/demo.ddsl
./ddsl_cli transpile examples/scripts/demo.ddsl
```

Búsqueda de símbolos prohibidos en `.c` y `.h`:

```sh
grep -RInE 'malloc|calloc|realloc|free|heap|double|float|strtod|snprintf|%\.17g|%\.6g' . --include='*.c' --include='*.h'
```

Resultado esperado: sin coincidencias.
