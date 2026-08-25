# var_dsl89

Mini-DSL de variables inspirado en la superficie de GML y agnóstico del host.
No almacena estado: emite comandos hacia un provider/runtime.

## Sintaxis

```text
hp = 100
hp += 25
hp -= 10

var damage = 12.5
var alive = true
var label = "temporary"

self.combo = 3
global.score = 1000
global.score += 50

foo = 3
var bonus = 2
foo += bonus
weapon.damage_multiplier = 2
```

Semántica:

- `name` / `self.name` -> INSTANCE
- `var name` -> LOCAL
- `global.name` -> GLOBAL
- `=`, `+=`, `-=`
- RHS literal o referencia a variable
- nombres con `.` permitidos
- buffer con sentencias separadas por newline o `;`
- comentarios `#` y `//` fuera de strings
- números decimales -> Q16.16 directamente, sin `float`/`double`
- strings -> buffers fijos

El host decide qué significa realmente cada scope y dónde vive el valor.
`var_runtime89` es el router recomendado, pero no es dependencia de este parser.

## Restricciones

- C89
- sin malloc/calloc/realloc/free
- sin heap propio
- sin float/double
- almacenamiento fijo
- Q16.16
