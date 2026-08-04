#include "bolt3d/b3d_projectile.h"

void b3d_projectile_def_clear(B3D_ProjectileDef *def_value)
{
    if (def_value == 0) {
        return;
    }

    def_value->kind = B3D_PROJECTILE_KIND_GENERIC;
    def_value->flags = 0;
    def_value->hit_mask = B3D_LAYER_ALL;
    def_value->damage_type = B3D_DAMAGE_GENERIC;
    def_value->radius = 0;
    def_value->speed = 0;
    def_value->damage = 0;
    def_value->stun = 0;
    def_value->knockback = 0;
    def_value->lifetime = b3d_fixed_from_int(4);
    def_value->gravity = 0;
    def_value->bounce = 0;
    def_value->max_distance = 0;
    def_value->owner_safe_time = 0;
    def_value->explosion_radius = 0;
    def_value->explosion_inner_radius = 0;
    def_value->explosion_force = 0;
    def_value->max_hits = 1;
    def_value->pierce_count = 0;
    def_value->user_ptr = 0;
}

void b3d_projectile_clear(B3D_Projectile *projectile)
{
    if (projectile == 0) {
        return;
    }

    projectile->active = B3D_FALSE;
    projectile->id = B3D_ID_NONE;
    projectile->slot = B3D_ID_NONE;
    projectile->kind = B3D_PROJECTILE_KIND_GENERIC;
    projectile->flags = 0;
    projectile->owner_id = B3D_ID_NONE;
    projectile->hit_mask = B3D_LAYER_ALL;
    projectile->damage_type = B3D_DAMAGE_GENERIC;
    projectile->hit_count = 0;
    projectile->max_hits = 0;
    projectile->remaining_pierces = 0;
    projectile->last_hit_id = B3D_ID_NONE;
    projectile->position = b3d_vec3_zero();
    projectile->previous_position = b3d_vec3_zero();
    projectile->velocity = b3d_vec3_zero();
    projectile->acceleration = b3d_vec3_zero();
    projectile->radius = 0;
    projectile->damage = 0;
    projectile->stun = 0;
    projectile->knockback = 0;
    projectile->age = 0;
    projectile->lifetime = 0;
    projectile->gravity = 0;
    projectile->bounce = 0;
    projectile->distance_traveled = 0;
    projectile->max_distance = 0;
    projectile->owner_safe_time = 0;
    projectile->explosion_radius = 0;
    projectile->explosion_inner_radius = 0;
    projectile->explosion_force = 0;
    projectile->user_ptr = 0;
}

B3D_ProjectileDef b3d_projectile_def_bullet(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_BULLET;
    def_value.flags = B3D_PROJ_OWNER_SAFE | B3D_PROJ_DIR_IS_NORMALIZED;
    def_value.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_ENEMY | B3D_LAYER_PROP;
    def_value.damage_type = B3D_DAMAGE_BULLET;
    def_value.radius = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(16));
    def_value.speed = b3d_fixed_from_int(60);
    def_value.damage = b3d_fixed_from_int(10);
    def_value.lifetime = b3d_fixed_from_int(2);
    def_value.max_distance = b3d_fixed_from_int(120);
    def_value.owner_safe_time = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(30));
    def_value.max_hits = 1;
    return def_value;
}

B3D_ProjectileDef b3d_projectile_def_rocket(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_ROCKET;
    def_value.flags = B3D_PROJ_EXPLODE_ON_HIT | B3D_PROJ_OWNER_SAFE | B3D_PROJ_DIR_IS_NORMALIZED;
    def_value.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_ENEMY | B3D_LAYER_PROP;
    def_value.damage_type = B3D_DAMAGE_EXPLOSION;
    def_value.radius = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(4));
    def_value.speed = b3d_fixed_from_int(20);
    def_value.damage = b3d_fixed_from_int(90);
    def_value.lifetime = b3d_fixed_from_int(6);
    def_value.max_distance = b3d_fixed_from_int(120);
    def_value.owner_safe_time = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(5));
    def_value.explosion_radius = b3d_fixed_from_int(6);
    def_value.explosion_inner_radius = b3d_fixed_from_int(2);
    def_value.explosion_force = b3d_fixed_from_int(35);
    def_value.max_hits = 1;
    return def_value;
}

B3D_ProjectileDef b3d_projectile_def_grenade(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_GRENADE;
    def_value.flags = B3D_PROJ_USE_GRAVITY | B3D_PROJ_BOUNCE | B3D_PROJ_EXPLODE_ON_HIT | B3D_PROJ_OWNER_SAFE | B3D_PROJ_DIR_IS_NORMALIZED;
    def_value.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_ENEMY | B3D_LAYER_PROP;
    def_value.damage_type = B3D_DAMAGE_EXPLOSION;
    def_value.radius = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(3));
    def_value.speed = b3d_fixed_from_int(12);
    def_value.damage = b3d_fixed_from_int(65);
    def_value.lifetime = b3d_fixed_from_int(4);
    def_value.gravity = b3d_fixed_from_int(12);
    def_value.bounce = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(2));
    def_value.owner_safe_time = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(4));
    def_value.explosion_radius = b3d_fixed_from_int(5);
    def_value.explosion_inner_radius = b3d_fixed_from_int(1);
    def_value.explosion_force = b3d_fixed_from_int(20);
    def_value.max_hits = 1;
    return def_value;
}

B3D_ProjectileDef b3d_projectile_def_plasma(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_PLASMA;
    def_value.flags = B3D_PROJ_OWNER_SAFE | B3D_PROJ_DIR_IS_NORMALIZED;
    def_value.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_ENEMY | B3D_LAYER_PROP;
    def_value.damage_type = B3D_DAMAGE_FIRE;
    def_value.radius = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(2));
    def_value.speed = b3d_fixed_from_int(10);
    def_value.damage = b3d_fixed_from_int(25);
    def_value.lifetime = b3d_fixed_from_int(5);
    def_value.max_distance = b3d_fixed_from_int(60);
    def_value.owner_safe_time = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(10));
    def_value.max_hits = 1;
    return def_value;
}

B3D_ProjectileDef b3d_projectile_def_melee_trace(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_MELEE_TRACE;
    def_value.flags = 0;
    def_value.hit_mask = B3D_LAYER_ENEMY | B3D_LAYER_PROP;
    def_value.damage_type = B3D_DAMAGE_SLASH;
    def_value.radius = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(2));
    def_value.speed = 0;
    def_value.damage = b3d_fixed_from_int(12);
    def_value.lifetime = b3d_fixed_div(b3d_fixed_from_int(1), b3d_fixed_from_int(20));
    def_value.max_hits = 4;
    def_value.pierce_count = 4;
    return def_value;
}

B3D_ProjectileDef b3d_projectile_def_solver_object(void)
{
    B3D_ProjectileDef def_value;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_CUSTOM;
    def_value.flags = B3D_PROJ_EMIT_MOVE;
    def_value.hit_mask = B3D_LAYER_ALL;
    def_value.radius = b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(2));
    def_value.lifetime = 0;
    def_value.max_distance = 0;
    def_value.max_hits = 0;
    return def_value;
}
