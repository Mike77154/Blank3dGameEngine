#include <stdio.h>

#include "ecs89.h"

typedef struct Position
{
    float x;
    float y;
} Position;

typedef struct Velocity
{
    float x;
    float y;
} Velocity;

typedef struct Health
{
    int hp;
} Health;

enum
{
    COMP_POSITION = 0,
    COMP_VELOCITY,
    COMP_HEALTH,
    COMP_PLAYER,
    COMP_COUNT
};

static Position g_positions[ECS_MAX_ENTITIES];
static Velocity g_velocities[ECS_MAX_ENTITIES];
static Health g_health[ECS_MAX_ENTITIES];

static void movement_system(ecs_world *world, float dt)
{
    ecs_view view;
    ecs_entity entity;
    Position *position;
    Velocity *velocity;
    ecs_mask require;

    require = ecs_component_bit(COMP_POSITION) | ecs_component_bit(COMP_VELOCITY);
    ecs_view_init(&view, world, require, (ecs_mask)0);

    for (;;)
    {
        entity = ecs_view_next(&view);
        if (ecs_entity_is_null(entity)) break;

        position = ECS_GET_AS(Position, world, entity, COMP_POSITION);
        velocity = ECS_GET_AS(Velocity, world, entity, COMP_VELOCITY);

        position->x += velocity->x * dt;
        position->y += velocity->y * dt;
    }
}

static void damage_system(ecs_world *world, int damage)
{
    ecs_view view;
    ecs_entity entity;
    Health *health;
    ecs_mask require;

    require = ecs_component_bit(COMP_HEALTH);
    ecs_view_init(&view, world, require, (ecs_mask)0);

    for (;;)
    {
        entity = ecs_view_next(&view);
        if (ecs_entity_is_null(entity)) break;

        health = ECS_GET_AS(Health, world, entity, COMP_HEALTH);
        health->hp -= damage;

        if (health->hp <= 0)
        {
            ecs_entity_destroy(world, entity);
        }
    }
}

int main(void)
{
    ecs_world world;
    ecs_component_desc desc;
    ecs_entity player;
    ecs_entity enemy;
    Position position;
    Velocity velocity;
    Health health;
    Position *player_position;
    Position *enemy_position;

    if (!ecs_world_init(&world, 256))
    {
        printf("No se pudo iniciar ecs_world.\n");
        return 1;
    }

    desc.name = "Position";
    desc.size = sizeof(Position);
    desc.stride = 0u;
    desc.storage = g_positions;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_POSITION, &desc, (const ecs_component_hooks*)0)) return 1;

    desc.name = "Velocity";
    desc.size = sizeof(Velocity);
    desc.stride = 0u;
    desc.storage = g_velocities;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_VELOCITY, &desc, (const ecs_component_hooks*)0)) return 1;

    desc.name = "Health";
    desc.size = sizeof(Health);
    desc.stride = 0u;
    desc.storage = g_health;
    desc.flags = ECS_COMPONENT_FLAG_NONE;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_HEALTH, &desc, (const ecs_component_hooks*)0)) return 1;

    desc.name = "PlayerTag";
    desc.size = 0u;
    desc.stride = 0u;
    desc.storage = (void*)0;
    desc.flags = ECS_COMPONENT_FLAG_TAG;
    desc.user_data = (void*)0;
    if (!ecs_register_component(&world, COMP_PLAYER, &desc, (const ecs_component_hooks*)0)) return 1;

    player = ecs_entity_create(&world);
    enemy = ecs_entity_create(&world);

    position.x = 10.0f;
    position.y = 5.0f;
    velocity.x = 2.0f;
    velocity.y = -1.0f;
    health.hp = 100;

    ecs_add_component(&world, player, COMP_POSITION, &position);
    ecs_add_component(&world, player, COMP_VELOCITY, &velocity);
    ecs_add_component(&world, player, COMP_HEALTH, &health);
    ecs_add_tag(&world, player, COMP_PLAYER);

    position.x = -3.0f;
    position.y = 8.0f;
    velocity.x = 0.25f;
    velocity.y = 0.5f;
    health.hp = 40;

    ecs_add_component(&world, enemy, COMP_POSITION, &position);
    ecs_add_component(&world, enemy, COMP_VELOCITY, &velocity);
    ecs_add_component(&world, enemy, COMP_HEALTH, &health);

    movement_system(&world, 3.0f);
    damage_system(&world, 50);

    player_position = ECS_GET_AS(Position, &world, player, COMP_POSITION);
    enemy_position = ECS_GET_AS(Position, &world, enemy, COMP_POSITION);

    printf("Vivos: %d\n", ecs_count_alive(&world));

    if (player_position != (Position*)0)
    {
        printf("Player pos = (%f, %f)\n", player_position->x, player_position->y);
    }

    if (ecs_entity_valid(&world, player))
    {
        printf("Player sigue en pie.\n");
    }

    if (ecs_entity_valid(&world, enemy))
    {
        printf("Enemy sobrevivio.\n");
        if (enemy_position != (Position*)0)
        {
            printf("Enemy pos = (%f, %f)\n", enemy_position->x, enemy_position->y);
        }
    }
    else
    {
        printf("Enemy fue destruido.\n");
    }

    return 0;
}
