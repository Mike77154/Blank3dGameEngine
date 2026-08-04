# Resultado del parche DDSL2 0.3.2

La biblioteca fue corregida y validada con:

```sh
make check
```

Compilación estricta:

```text
gcc -std=c89 -pedantic -Wall -Wextra -Werror
clang -std=c89 -pedantic -Wall -Wextra -Werror
```

## Cambios de 0.3.2

1. Prototipos públicos para la API de versión.
2. Macros completas de versión de paquete.
3. Validador semántico real para AST, expresiones, acciones y cláusulas.
4. Protección contra AST cíclicos, hostiles o excesivamente profundos.
5. Rechazo de identificadores que el store habría truncado.
6. Rechazo de strings que el store habría truncado.
7. Protección de los literales reservados `true` y `false`.
8. Validación automática antes de construir IR o ejecutar un AST.
9. Pruebas de regresión para los nuevos contratos.
10. Aviso explícito sobre la ausencia de una licencia verificable en el paquete recibido.

## Correcciones heredadas de 0.3.1

- acciones sin coma rechazadas;
- números inválidos rechazados;
- cadenas `if/elif` sin límite oculto de 64 saltos;
- bytecode propietario de sus literales;
- errores del store propagados;
- escapes UTF-8 seguros en C transpilado;
- creación automática del directorio `build/`.

Consulta `docs/PATCH_0.3.2.md`, `docs/PATCH_0.3.1.md` y `tests/test_regressions.c`.
