#include <stdio.h>
#include <string.h>

#include "ecs89.h"

typedef struct Position
{
    int x;
    int y;
} Position;

typedef struct Velocity
{
    int x;
    int y;
} Velocity;

typedef struct HookStats
{
    int add_count;
    int set_count;
    int remove_count;
} HookStats;

typedef struct WorldStats
{
    int created;
    int destroyed;
} WorldStats;

typedef struct ExternalThing
{
    int value;
} ExternalThing;

enum
{
    COMP_POSITION = 0,
    COMP_VELOCITY,
    COMP_TAG,
    COMP_PTR
};

static Position g_positions[ECS_MAX_ENTITIES];
static Velocity g_velocities[ECS_MAX_ENTITIES];
static void *g_pointer_bindings[ECS_MAX_ENTITIES];

static void on_comp_add(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data)
{
    HookStats *stats;

    (void)world;
    (void)entity;
    (void)component_id;
    (void)component_ptr;

    stats = (HookStats*)user_data;
    stats->add_count += 1;
}

static void on_comp_set(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data)
{
    HookStats *stats;

    (void)world;
    (void)entity;
    (void)component_id;
    (void)component_ptr;

    stats = (HookStats*)user_data;
    stats->set_count += 1;
}

static void on_comp_remove(ecs_world *world, ecs_entity entity, int component_id, void *component_ptr, void *user_data)
{
    HookStats *stats;

    (void)world;
    (void)entity;
    (void)component_id;
    (void)component_ptr;

    stats = (HookStats*)user_data;
    stats->remove_count += 1;
}

static void on_entity_create(ecs_world *world, ecs_entity entity, void *user_data)
{
    WorldStats *stats;

    (void)world;
    (void)entity;

    stats = (WorldStats*)user_data;
    stats->created += 1;
}

static void on_entity_destroy(ecs_world *world, ecs_entity entity, void *user_data)
{
    WorldStats *stats;

    (void)world;
    (void)entity;

    stats = (WorldStats*)user_data;
    stats->destroyed += 1;
}

static int check_true(int expr, const char *message)
{
    if (!expr)
    {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    ecs_world world;
    ecs_component_desc desc;
    ecs_component_hooks comp_hooks;
    ecs_world_hooks world_hooks;
    ecs_entity first;
    ecs_entity second;
    Position pos;
    Velocity vel;
    Position *pos_ptr;
    Velocity *vel_ptr;
    HookStats hook_stats;
    WorldStats world_stats;
    ExternalThing external;
    int ok;
    ecs_view view;
    ecs_entity iter;
    int matches;
    char *world_name;

    hook_stats.add_count = 0;
    hook_stats.set_count = 0;
    hook_stats.remove_count = 0;

    world_stats.created = 0;
    world_stats.destroyed = 0;

    external.value = 77;
    ok = 1;

    if (!ecs_world_init(&world, 64))
    {
        printf("FAIL: ecs_world_init\n");
        return 1;
    }

    world_name = "test-world";
    ecs_world_set_user_data(&world, world_name);

    memset(&world_hooks, 0, sizeof(world_hooks));
    world_hooks.on_entity_create = on_entity_create;
    world_hooks.on_entity_destroy = on_entity_destroy;
    ecs_world_set_hooks(&world, &world_hooks, &world_stats);

    memset(&comp_hooks, 0, sizeof(comp_hooks));
    comp_hooks.on_add = on_comp_add;
    comp_hooks.on_set = on_comp_set;
    comp_hooks.on_remove = on_comp_remove;

    desc.name = "Position";
    desc.size = sizeof(Position);
    desc.stride = 0u;
    desc.storage = g_positions;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = &hook_stats;
    ok = ok && check_true(ecs_register_component(&world, COMP_POSITION, &desc, &comp_hooks), "register position");

    desc.name = "Velocity";
    desc.size = sizeof(Velocity);
    desc.stride = 0u;
    desc.storage = g_velocities;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = &hook_stats;
    ok = ok && check_true(ecs_register_component(&world, COMP_VELOCITY, &desc, &comp_hooks), "register velocity");

    desc.name = "Tag";
    desc.size = 0u;
    desc.stride = 0u;
    desc.storage = (void*)0;
    desc.flags = ECS_COMPONENT_FLAG_TAG;
    desc.user_data = &hook_stats;
    ok = ok && check_true(ecs_register_component(&world, COMP_TAG, &desc, &comp_hooks), "register tag");

    desc.name = "Ptr";
    desc.size = sizeof(void*);
    desc.stride = 0u;
    desc.storage = g_pointer_bindings;
    desc.flags = ECS_COMPONENT_FLAG_POINTER;
    desc.user_data = &hook_stats;
    ok = ok && check_true(ecs_register_component(&world, COMP_PTR, &desc, &comp_hooks), "register ptr");

    first = ecs_entity_create(&world);
    ok = ok && check_true(!ecs_entity_is_null(first), "first entity valid");
    ok = ok && check_true(world_stats.created == 1, "entity create hook");

    pos.x = 10;
    pos.y = 20;
    vel.x = 3;
    vel.y = -2;

    ok = ok && check_true(ecs_add_component(&world, first, COMP_POSITION, &pos), "add position");
    ok = ok && check_true(ecs_add_component(&world, first, COMP_VELOCITY, &vel), "add velocity");
    ok = ok && check_true(ecs_add_tag(&world, first, COMP_TAG), "add tag");
    ok = ok && check_true(ecs_bind_ptr(&world, first, COMP_PTR, &external), "bind pointer");

    ok = ok && check_true(ecs_has_component(&world, first, COMP_POSITION), "has position");
    ok = ok && check_true(ecs_has_component(&world, first, COMP_TAG), "has tag");
    ok = ok && check_true(ecs_get_bound_ptr(&world, first, COMP_PTR) == &external, "get bound ptr");

    pos_ptr = ECS_GET_AS(Position, &world, first, COMP_POSITION);
    vel_ptr = ECS_ASSIGN_AS(Velocity, &world, first, COMP_VELOCITY);
    pos_ptr->x += vel_ptr->x;
    pos_ptr->y += vel_ptr->y;

    ok = ok && check_true(pos_ptr->x == 13 && pos_ptr->y == 18, "position updated");

    vel.x = 9;
    vel.y = 9;
    ok = ok && check_true(ecs_set_component(&world, first, COMP_VELOCITY, &vel), "set velocity");
    ok = ok && check_true(hook_stats.set_count >= 1, "set hook fired");

    view.world = (ecs_world*)0;
    view.require_all = (ecs_mask)0;
    view.exclude_any = (ecs_mask)0;
    view.cursor = 0;

    ecs_view_init(&view,
                  &world,
                  ecs_component_bit(COMP_POSITION) | ecs_component_bit(COMP_VELOCITY),
                  (ecs_mask)0);

    iter = ecs_view_next(&view);
    ok = ok && check_true(!ecs_entity_is_null(iter), "view finds entity");
    ok = ok && check_true(iter.index == first.index, "view returns first");

    matches = ecs_count_matching(&world,
                                 ecs_component_bit(COMP_POSITION),
                                 (ecs_mask)0);
    ok = ok && check_true(matches == 1, "count matching");

    ok = ok && check_true(ecs_remove_component(&world, first, COMP_TAG), "remove tag");
    ok = ok && check_true(!ecs_has_component(&world, first, COMP_TAG), "tag removed");

    ecs_entity_destroy(&world, first);
    ok = ok && check_true(!ecs_entity_valid(&world, first), "old handle invalid");
    ok = ok && check_true(world_stats.destroyed == 1, "entity destroy hook");
    ok = ok && check_true(hook_stats.remove_count >= 3, "remove hooks fired on destroy");

    second = ecs_entity_create(&world);
    ok = ok && check_true(!ecs_entity_is_null(second), "second entity valid");
    ok = ok && check_true(second.generation != first.generation || second.index != first.index,
                          "generation or slot changed");

    ok = ok && check_true(ecs_count_alive(&world) == 1, "one entity alive");

    ok = ok && check_true(strcmp((const char*)ecs_world_get_user_data(&world), "test-world") == 0,
                          "world user data");

    ecs_world_reset(&world);
    ok = ok && check_true(ecs_count_alive(&world) == 0, "reset empties world");

    if (!ok)
    {
        return 1;
    }

    printf("OK: ecs89 tests passed\n");
    return 0;
}
