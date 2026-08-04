# ecs89_megasystem

ECS en **C89 puro**, sin `malloc`, pensado para ser:

- sólido
- fácil de extender
- fácil de integrar con librerías externas
- fácil de “enchufar” con callbacks, servicios y componentes puntero

## Qué trae

- **entidades con generación** para invalidar handles viejos
- **componentes estáticos** con storage provisto por el usuario
- **tags** sin payload
- **componentes puntero** para bindear objetos externos
- **hooks** por componente: `on_add`, `on_set`, `on_remove`
- **hooks** globales del mundo: create/destroy de entidad
- **service slots** para colgar renderers, físicas, audio, scripting, etc.
- **views** por máscara `require_all` + `exclude_any`
- **sin heap**
- **sin dependencia rara**
- **compila como librería estática**

## Estructura

```text
ecs89_megasystem/
├── include/
│   └── ecs89.h
├── src/
│   └── ecs89.c
├── examples/
│   ├── demo_basic.c
│   └── demo_integration.c
├── tests/
│   └── test_ecs89.c
├── Makefile
├── LICENSE
└── README.md
```

## Build

```bash
make
make test
make run-basic
make run-integration
```

## Uso rápido

```c
#include "ecs89.h"

typedef struct Position
{
    float x;
    float y;
} Position;

enum
{
    COMP_POSITION = 0,
    COMP_PLAYER_TAG
};

static Position g_positions[ECS_MAX_ENTITIES];

int main(void)
{
    ecs_world world;
    ecs_component_desc desc;
    ecs_entity entity;
    Position *position;

    ecs_world_init(&world, 256);

    desc.name = "Position";
    desc.size = sizeof(Position);
    desc.stride = 0u;
    desc.storage = g_positions;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    ecs_register_component(&world, COMP_POSITION, &desc, (const ecs_component_hooks*)0);

    desc.name = "PlayerTag";
    desc.size = 0u;
    desc.stride = 0u;
    desc.storage = (void*)0;
    desc.flags = ECS_COMPONENT_FLAG_TAG;
    desc.user_data = (void*)0;
    ecs_register_component(&world, COMP_PLAYER_TAG, &desc, (const ecs_component_hooks*)0);

    entity = ecs_entity_create(&world);

    position = ECS_ASSIGN_AS(Position, &world, entity, COMP_POSITION);
    position->x = 10.0f;
    position->y = 20.0f;

    ecs_add_tag(&world, entity, COMP_PLAYER_TAG);
    return 0;
}
```

---

## Cómo extenderlo fácil

### 1. Agregar más componentes

Solo creas un pool estático nuevo y lo registras.

```c
typedef struct Mana
{
    int value;
} Mana;

static Mana g_mana[ECS_MAX_ENTITIES];
```

```c
desc.name = "Mana";
desc.size = sizeof(Mana);
desc.stride = 0u;
desc.storage = g_mana;
desc.flags = ECS_COMPONENT_FLAG_NONE;
desc.user_data = (void*)0;
ecs_register_component(&world, COMP_MANA, &desc, (const ecs_component_hooks*)0);
```

### 2. Hookear librerías externas

Puedes colgar punteros a servicios en el mundo.

```c
enum
{
    SERVICE_RENDERER = 0,
    SERVICE_PHYSICS,
    SERVICE_AUDIO
};

ecs_world_set_service(&world, SERVICE_RENDERER, &renderer);
ecs_world_set_service(&world, SERVICE_PHYSICS, &physics);
ecs_world_set_service(&world, SERVICE_AUDIO, &audio);
```

Luego tus systems los leen directo:

```c
Renderer *renderer;
renderer = (Renderer*)ecs_world_get_service(&world, SERVICE_RENDERER);
```

### 3. Bindings a punteros

Para referenciar objetos externos por entidad, registra un componente con `ECS_COMPONENT_FLAG_POINTER`.

```c
static void *g_sprite_links[ECS_MAX_ENTITIES];

desc.name = "SpritePtr";
desc.size = sizeof(void*);
desc.stride = 0u;
desc.storage = g_sprite_links;
desc.flags = ECS_COMPONENT_FLAG_POINTER;
desc.user_data = (void*)0;
ecs_register_component(&world, COMP_SPRITE_PTR, &desc, (const ecs_component_hooks*)0);
```

Bind:

```c
ecs_bind_ptr(&world, entity, COMP_SPRITE_PTR, sprite_ptr);
```

Read:

```c
Sprite *sprite;
sprite = (Sprite*)ecs_get_bound_ptr(&world, entity, COMP_SPRITE_PTR);
```

### 4. Hooks de ciclo de vida

Cada componente puede disparar callbacks:

- `on_add`
- `on_set`
- `on_remove`

Muy útil para:

- sincronizar con renderer
- crear/destruir handles externos
- loguear
- invalidar caches
- disparar scripting
- puentear ECS ↔ motor externo

```c
static void on_sprite_removed(ecs_world *world,
                              ecs_entity entity,
                              int component_id,
                              void *component_ptr,
                              void *user_data)
{
    Renderer *renderer;
    renderer = (Renderer*)ecs_world_get_service(world, SERVICE_RENDERER);
    (void)entity;
    (void)component_id;
    (void)component_ptr;
    (void)user_data;

    if (renderer != (Renderer*)0)
    {
        renderer->detach_count += 1;
    }
}
```

### 5. Excluir componentes en queries

```c
ecs_view view;
ecs_entity entity;
ecs_mask require;
ecs_mask exclude;

require = ecs_component_bit(COMP_POSITION) | ecs_component_bit(COMP_VELOCITY);
exclude = ecs_component_bit(COMP_SLEEPING);

ecs_view_init(&view, &world, require, exclude);

for (;;)
{
    entity = ecs_view_next(&view);
    if (ecs_entity_is_null(entity)) break;

    /* system body */
}
```

---

## Ideas de integración directa

Este core está preparado para colgarle sin dolor:

- renderer
- físicas
- audio
- IA
- scripting
- network replication
- save/load
- debug overlays
- inspector
- profiler
- job system externo
- wrappers C++ arriba del C89

## Decisiones de diseño

### Por qué no usa `malloc`

Para que sirva bien en:

- consolas o plataformas estrictas
- embedded
- binarios pequeños
- entornos deterministas
- engines propios que controlan toda la memoria

### Por qué usa storage externo

Porque así el usuario decide:

- layout
- ubicación
- alineación práctica
- ownership
- integración con pools ya existentes

### Por qué tiene service slots

Para no acoplar el core a ninguna lib concreta y, al mismo tiempo, permitir inyección facilísima de dependencias.

---

## Limitaciones conocidas

- máscara basada en `ecs_mask`, por defecto cómoda para hasta **32 componentes**
- storage por índice de entidad, no archetypes/chunks
- no trae scheduler ni command buffer todavía
- no trae serialización integrada todavía

## Siguiente escalón natural

Si quieres evolucionarlo después, el camino lógico es:

1. command buffer diferido
2. sparse-set por componente
3. query cache
4. serialización binaria / snapshots
5. módulos opcionales (`ecs89_events`, `ecs89_snapshot`, `ecs89_debug`)
6. bridge con Lua / Python / C++

---

## Verificado

Este paquete fue preparado para compilar con:

```bash
cc -std=c89 -pedantic -Wall -Wextra -Werror
```

y trae test ejecutable para validar el core.
