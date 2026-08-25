# var_manager89 — runtime-created variables


## Regla central: cero variables hardcodeadas

El manager **no conoce ningún nombre de variable al compilarse**.

Al iniciar:

```text
globals   = vacío
instances = vacío
locals    = vacío
```

La capacidad de las tablas es estática, pero **sus miembros no lo son**. Esto:

```c
vm89_set(&m, VM89_SCOPE_GLOBAL, 0UL, user_supplied_name, &value);
```

puede recibir en runtime:

```text
"score"
"pepito"
"gravity_mode"
"quest_818_status"
"cualquier_nombre_creado_por_un_DSL"
```

Si no existe, `SET` toma un slot libre, copia el nombre al buffer fijo y crea la variable ahí mismo.

No existe:

```c
struct {
    int hp;
    int score;
    int ammo;
};
```

ni enums de variables conocidas, ni IDs compilados para cada variable, ni schemas obligatorios.

### Qué sí está fijado en compilación

Sólo la **infraestructura máxima**:

```text
cantidad máxima de globals
cantidad máxima de instancias
variables máximas por instancia
frames locales máximos
variables locales máximas por frame
longitud máxima del nombre
longitud máxima de string
```

Eso respeta la regla sin heap: el almacén existe de antemano, pero está **vacío y sin significado**.

## API genérica

```c
vm89_set(...)
vm89_get(...)
vm89_exists(...)
vm89_unset(...)
```

Reciben el scope y el nombre en runtime.

También siguen disponibles las APIs específicas:

```c
vm89_global_set(...)
vm89_instance_set(...)
vm89_local_set(...)
```

Son sólo atajos; tampoco contienen nombres predefinidos.

## Creación en caliente

```c
char dynamic_name[VM89_NAME_MAX];

/* El host/DSL construye o recibe el nombre. */
strcpy(dynamic_name, "energia_del_portal");

vm89_value_fixed_int(&v, 900);
vm89_set(&manager, VM89_SCOPE_GLOBAL, 0UL, dynamic_name, &v);
```

La variable `energia_del_portal` no existía al compilar ni al inicializar el manager.

## Destrucción en caliente

```c
vm89_unset(&manager, VM89_SCOPE_GLOBAL, 0UL, "energia_del_portal");
```

El slot vuelve a quedar libre y puede reutilizarse con otro nombre.




Gestor de variables con scopes inspirados en GameMaker, sin depender de GML ni de ningún parser.

## Modelo

```text
vm89_manager
├── globals[]                       compartidas
├── instances[]
│   └── instance.vars[]             persisten por instancia
└── frames[]                        pila de eventos/funciones
    └── locals[]                    mueren al hacer end_event()
```

### GLOBAL

```c
vm89_global_set(&m, "score", &v);
vm89_global_get(&m, "score", &out);
```

### INSTANCE

```c
vm89_instance_create(&m, player_id);
vm89_instance_set(&m, player_id, "hp", &v);
```

Cada ID posee su propia tabla fija.

### LOCAL / EVENTO

```c
vm89_begin_event(&m, player_id, event_id);
vm89_local_set(&m, "damage", &v);

/* ejecutar evento */

vm89_end_event(&m); /* limpia todos los locals de ese frame */
```

Los frames pueden anidarse para representar llamadas de funciones/eventos.

## Resolución no calificada

`vm89_resolve()` busca:

```text
LOCAL actual
   ↓
INSTANCE actual
```

No cae implícitamente a GLOBAL: las globales se consultan explícitamente, igual que la intención de `global.nombre`.

## Valores

Tagged value fijo:

- Q16.16
- bool
- string de buffer fijo

No hay `float`, `double` ni memoria dinámica.

## Capacidades configurables

Antes de incluir el header puedes redefinir:

```c
#define VM89_MAX_GLOBALS 128
#define VM89_MAX_INSTANCES 64
#define VM89_MAX_INSTANCE_VARS 64
#define VM89_MAX_LOCAL_FRAMES 16
#define VM89_MAX_LOCALS_PER_FRAME 64
```

## Build

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude src/var_manager89.c examples/demo.c -o vm89_demo
```

## Independencia

`var_manager89` no incluye ni conoce `var_dsl89`.
Puede ser usado directamente por C, otro DSL, un event sheet, un VM, etc.
