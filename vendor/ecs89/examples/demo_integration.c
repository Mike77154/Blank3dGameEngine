#include <stdio.h>
#include <string.h>

#include "ecs89.h"

typedef struct Position
{
    float x;
    float y;
} Position;

typedef struct Sprite
{
    int sprite_id;
    float x;
    float y;
} Sprite;

typedef struct Body
{
    int body_id;
    float x;
    float y;
} Body;

typedef struct Renderer
{
    int sync_count;
    int detach_count;
} Renderer;

typedef struct PhysicsWorld
{
    int pull_count;
    int detach_count;
} PhysicsWorld;

enum
{
    SERVICE_RENDERER = 0,
    SERVICE_PHYSICS
};

enum
{
    COMP_POSITION = 0,
    COMP_SPRITE_PTR,
    COMP_BODY_PTR,
    COMP_DEBUG_NAME,
    COMP_COUNT
};

static Position g_positions[ECS_MAX_ENTITIES];
static void *g_sprite_bindings[ECS_MAX_ENTITIES];
static void *g_body_bindings[ECS_MAX_ENTITIES];
static char g_debug_names[ECS_MAX_ENTITIES][32];

static void on_sprite_removed(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data)
{
    Renderer *renderer;
    Sprite *sprite;
    void *raw;

    (void)component_id;
    (void)user_data;

    renderer = (Renderer*)ecs_world_get_service(world, SERVICE_RENDERER);
    sprite = (Sprite*)0;
    raw = (void*)0;

    if (component_ptr != (void*)0)
    {
        memcpy(&raw, component_ptr, sizeof(void*));
        sprite = (Sprite*)raw;
    }

    if (renderer != (Renderer*)0)
    {
        renderer->detach_count += 1;
    }

    if (sprite != (Sprite*)0)
    {
        printf("[hook] sprite detach entity=%u sprite_id=%d\n",
               (unsigned int)entity.index,
               sprite->sprite_id);
    }
}

static void on_body_removed(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data)
{
    PhysicsWorld *physics;
    Body *body;
    void *raw;

    (void)component_id;
    (void)user_data;

    physics = (PhysicsWorld*)ecs_world_get_service(world, SERVICE_PHYSICS);
    body = (Body*)0;
    raw = (void*)0;

    if (component_ptr != (void*)0)
    {
        memcpy(&raw, component_ptr, sizeof(void*));
        body = (Body*)raw;
    }

    if (physics != (PhysicsWorld*)0)
    {
        physics->detach_count += 1;
    }

    if (body != (Body*)0)
    {
        printf("[hook] body detach entity=%u body_id=%d\n",
               (unsigned int)entity.index,
               body->body_id);
    }
}

static void render_sync_system(ecs_world *world)
{
    ecs_view view;
    ecs_entity entity;
    Position *position;
    Sprite *sprite;
    Renderer *renderer;
    ecs_mask require;

    renderer = (Renderer*)ecs_world_get_service(world, SERVICE_RENDERER);
    require = ecs_component_bit(COMP_POSITION) | ecs_component_bit(COMP_SPRITE_PTR);

    ecs_view_init(&view, world, require, (ecs_mask)0);

    for (;;)
    {
        entity = ecs_view_next(&view);
        if (ecs_entity_is_null(entity)) break;

        position = ECS_GET_AS(Position, world, entity, COMP_POSITION);
        sprite = (Sprite*)ecs_get_bound_ptr(world, entity, COMP_SPRITE_PTR);
        if (position == (Position*)0 || sprite == (Sprite*)0) continue;

        sprite->x = position->x;
        sprite->y = position->y;

        if (renderer != (Renderer*)0)
        {
            renderer->sync_count += 1;
        }
    }
}

static void physics_pull_system(ecs_world *world)
{
    ecs_view view;
    ecs_entity entity;
    Position *position;
    Body *body;
    PhysicsWorld *physics;
    ecs_mask require;

    physics = (PhysicsWorld*)ecs_world_get_service(world, SERVICE_PHYSICS);
    require = ecs_component_bit(COMP_POSITION) | ecs_component_bit(COMP_BODY_PTR);

    ecs_view_init(&view, world, require, (ecs_mask)0);

    for (;;)
    {
        entity = ecs_view_next(&view);
        if (ecs_entity_is_null(entity)) break;

        position = ECS_GET_AS(Position, world, entity, COMP_POSITION);
        body = (Body*)ecs_get_bound_ptr(world, entity, COMP_BODY_PTR);
        if (position == (Position*)0 || body == (Body*)0) continue;

        position->x = body->x;
        position->y = body->y;

        if (physics != (PhysicsWorld*)0)
        {
            physics->pull_count += 1;
        }
    }
}

int main(void)
{
    ecs_world world;
    ecs_component_desc desc;
    ecs_component_hooks hooks;
    ecs_entity entity;
    Position *position;
    Renderer renderer;
    PhysicsWorld physics;
    Sprite sprite;
    Body body;
    char name_buffer[32];

    renderer.sync_count = 0;
    renderer.detach_count = 0;

    physics.pull_count = 0;
    physics.detach_count = 0;

    sprite.sprite_id = 7;
    sprite.x = 0.0f;
    sprite.y = 0.0f;

    body.body_id = 99;
    body.x = 32.0f;
    body.y = 48.0f;

    if (!ecs_world_init(&world, 128))
    {
        printf("No se pudo iniciar ecs_world.\n");
        return 1;
    }

    ecs_world_set_service(&world, SERVICE_RENDERER, &renderer);
    ecs_world_set_service(&world, SERVICE_PHYSICS, &physics);

    desc.name = "Position";
    desc.size = sizeof(Position);
    desc.stride = 0u;
    desc.storage = g_positions;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_POSITION, &desc, (const ecs_component_hooks*)0)) return 1;

    memset(&hooks, 0, sizeof(hooks));
    hooks.on_remove = on_sprite_removed;

    desc.name = "SpritePtr";
    desc.size = sizeof(void*);
    desc.stride = 0u;
    desc.storage = g_sprite_bindings;
    desc.flags = ECS_COMPONENT_FLAG_POINTER;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_SPRITE_PTR, &desc, &hooks)) return 1;

    memset(&hooks, 0, sizeof(hooks));
    hooks.on_remove = on_body_removed;

    desc.name = "BodyPtr";
    desc.size = sizeof(void*);
    desc.stride = 0u;
    desc.storage = g_body_bindings;
    desc.flags = ECS_COMPONENT_FLAG_POINTER;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_BODY_PTR, &desc, &hooks)) return 1;

    desc.name = "DebugName";
    desc.size = 32u;
    desc.stride = 32u;
    desc.storage = g_debug_names;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_DEBUG_NAME, &desc, (const ecs_component_hooks*)0)) return 1;

    entity = ecs_entity_create(&world);

    position = ECS_ASSIGN_AS(Position, &world, entity, COMP_POSITION);
    position->x = 4.0f;
    position->y = 5.0f;

    strcpy(name_buffer, "npc.bridge");
    ecs_add_component(&world, entity, COMP_DEBUG_NAME, name_buffer);

    ecs_bind_ptr(&world, entity, COMP_SPRITE_PTR, &sprite);
    ecs_bind_ptr(&world, entity, COMP_BODY_PTR, &body);

    render_sync_system(&world);
    printf("sprite synced -> (%f, %f)\n", sprite.x, sprite.y);

    body.x = 100.0f;
    body.y = 200.0f;
    physics_pull_system(&world);

    position = ECS_GET_AS(Position, &world, entity, COMP_POSITION);
    printf("position pulled from body -> (%f, %f)\n", position->x, position->y);

    printf("renderer syncs=%d physics pulls=%d\n", renderer.sync_count, physics.pull_count);

    ecs_entity_destroy(&world, entity);
    printf("renderer detaches=%d physics detaches=%d\n",
           renderer.detach_count,
           physics.detach_count);

    return 0;
}
