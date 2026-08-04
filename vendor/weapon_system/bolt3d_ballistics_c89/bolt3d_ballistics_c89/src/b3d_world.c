#include "bolt3d/b3d_world.h"

static void b3d_make_projectile_event(B3D_Event *event_value, int type_value, const B3D_Projectile *projectile)
{
    b3d_event_clear(event_value);
    event_value->type = type_value;
    if (projectile != 0) {
        event_value->projectile_id = projectile->id;
        event_value->projectile_slot = projectile->slot;
        event_value->owner_id = projectile->owner_id;
        event_value->damage_type = projectile->damage_type;
        event_value->damage = projectile->damage;
        event_value->stun = projectile->stun;
        event_value->knockback = projectile->knockback;
        event_value->position = projectile->position;
        event_value->previous_position = projectile->previous_position;
        event_value->velocity = projectile->velocity;
        event_value->flags = projectile->flags;
        event_value->source_user_ptr = projectile->user_ptr;
        event_value->user_ptr = projectile->user_ptr;
    }
}

static void b3d_emit_projectile_event(B3D_World *world, int type_value, const B3D_Projectile *projectile)
{
    B3D_Event event_value;

    b3d_make_projectile_event(&event_value, type_value, projectile);
    b3d_world_push_event(world, &event_value);
}

static void b3d_emit_hit_event(B3D_World *world, int type_value, const B3D_Projectile *projectile, const B3D_Hit *hit)
{
    B3D_Event event_value;

    b3d_make_projectile_event(&event_value, type_value, projectile);
    if (hit != 0) {
        event_value.target_id = hit->target_id;
        event_value.target_slot = hit->target_slot;
        event_value.target_layer = hit->target_layer;
        event_value.target_user_type = hit->target_user_type;
        event_value.t = hit->t;
        event_value.position = hit->point;
        event_value.normal = hit->normal;
        event_value.target_user_ptr = hit->user_ptr;
        event_value.user_ptr = hit->user_ptr;
    }
    b3d_world_push_event(world, &event_value);
}

static void b3d_emit_damage_event(B3D_World *world, const B3D_Projectile *projectile, const B3D_Hit *hit, B3D_Fixed damage)
{
    B3D_Event event_value;

    b3d_make_projectile_event(&event_value, B3D_EVENT_DAMAGE, projectile);
    event_value.damage = damage;
    if (hit != 0) {
        event_value.target_id = hit->target_id;
        event_value.target_slot = hit->target_slot;
        event_value.target_layer = hit->target_layer;
        event_value.target_user_type = hit->target_user_type;
        event_value.t = hit->t;
        event_value.position = hit->point;
        event_value.normal = hit->normal;
        event_value.target_user_ptr = hit->user_ptr;
        event_value.user_ptr = hit->user_ptr;
    }
    b3d_world_push_event(world, &event_value);
}

static int b3d_candidate_allowed(B3D_World *world, const B3D_TraceRequest *request, const B3D_Collider *collider, int hit_kind, int target_slot)
{
    B3D_FilterRequest filter_request;

    if (collider == 0) {
        return B3D_FALSE;
    }
    if (!collider->active) {
        return B3D_FALSE;
    }
    if ((collider->layer_mask & request->hit_mask) == 0) {
        return B3D_FALSE;
    }

    if (world->callbacks.filter_hit != 0) {
        filter_request.projectile_id = request->projectile_id;
        filter_request.projectile_slot = request->projectile_slot;
        filter_request.owner_id = request->owner_id;
        filter_request.target_id = collider->id;
        filter_request.target_slot = target_slot;
        filter_request.target_layer = collider->layer_mask;
        filter_request.target_user_type = collider->user_type;
        filter_request.hit_kind = hit_kind;
        filter_request.hit_mask = request->hit_mask;
        filter_request.projectile_user_ptr = request->projectile_user_ptr;
        filter_request.target_user_ptr = collider->user_ptr;
        if (!world->callbacks.filter_hit(world->callbacks.user_data, &filter_request)) {
            return B3D_FALSE;
        }
    }

    return B3D_TRUE;
}

static int b3d_is_owner_safe(const B3D_Projectile *projectile, int target_id)
{
    if (projectile == 0) {
        return B3D_FALSE;
    }
    if ((projectile->flags & B3D_PROJ_OWNER_SAFE) == 0) {
        return B3D_FALSE;
    }
    if (projectile->owner_id != target_id) {
        return B3D_FALSE;
    }
    if (projectile->age >= projectile->owner_safe_time) {
        return B3D_FALSE;
    }
    return B3D_TRUE;
}

static void b3d_deactivate_projectile(B3D_World *world, B3D_Projectile *projectile)
{
    if (projectile == 0 || !projectile->active) {
        return;
    }

    b3d_emit_projectile_event(world, B3D_EVENT_DESTROY, projectile);
    projectile->active = B3D_FALSE;
    if (world != 0 && world->solvers != 0 && projectile->slot >= 0 && projectile->slot < world->solver_capacity) {
        b3d_solver_state_clear(&world->solvers[projectile->slot]);
        world->solvers[projectile->slot].projectile_slot = projectile->slot;
    }
}

static int b3d_find_free_projectile_slot(B3D_World *world)
{
    int i;

    if (world == 0 || world->projectiles == 0) {
        return B3D_ID_NONE;
    }

    for (i = 0; i < world->projectile_capacity; ++i) {
        if (!world->projectiles[i].active) {
            return i;
        }
    }

    return B3D_ID_NONE;
}

static int b3d_find_free_collider_slot(B3D_World *world)
{
    int i;

    if (world == 0 || world->colliders == 0) {
        return B3D_ID_NONE;
    }

    for (i = 0; i < world->collider_capacity; ++i) {
        if (!world->colliders[i].active) {
            return i;
        }
    }

    return B3D_ID_NONE;
}

static B3D_Fixed b3d_explosion_damage(const B3D_Projectile *projectile, B3D_Fixed distance)
{
    B3D_Fixed outer_radius;
    B3D_Fixed inner_radius;
    B3D_Fixed range;
    B3D_Fixed t;
    B3D_Fixed one_minus_t;

    outer_radius = projectile->explosion_radius;
    inner_radius = projectile->explosion_inner_radius;

    if (outer_radius <= 0) {
        return projectile->damage;
    }
    if (inner_radius < 0) {
        inner_radius = 0;
    }
    if (inner_radius > outer_radius) {
        inner_radius = outer_radius;
    }
    if (distance <= inner_radius) {
        return projectile->damage;
    }
    if (distance >= outer_radius) {
        return 0;
    }

    range = b3d_fixed_sub_sat(outer_radius, inner_radius);
    if (range <= B3D_FIXED_EPSILON) {
        return projectile->damage;
    }

    t = b3d_fixed_div(b3d_fixed_sub_sat(distance, inner_radius), range);
    t = b3d_fixed_clamp(t, 0, B3D_FIXED_ONE);
    one_minus_t = b3d_fixed_sub_sat(B3D_FIXED_ONE, t);
    return b3d_fixed_mul(projectile->damage, one_minus_t);
}

static int b3d_explosion_allowed(B3D_World *world, const B3D_Projectile *projectile, const B3D_Collider *collider, B3D_Vec3 origin)
{
    B3D_ExplosionRequest request;

    if (world->callbacks.filter_explosion == 0) {
        return B3D_TRUE;
    }

    request.projectile_id = projectile->id;
    request.owner_id = projectile->owner_id;
    request.target_id = collider->id;
    request.target_slot = B3D_ID_NONE;
    request.target_layer = collider->layer_mask;
    request.target_user_type = collider->user_type;
    request.origin = origin;
    request.target_center = collider->center;
    request.radius = projectile->explosion_radius;
    request.projectile_user_ptr = projectile->user_ptr;
    request.target_user_ptr = collider->user_ptr;

    return world->callbacks.filter_explosion(world->callbacks.user_data, &request);
}

void b3d_world_explode_projectile(B3D_World *world, B3D_Projectile *projectile, B3D_Vec3 origin)
{
    B3D_Event explode_event;
    B3D_Hit hit;
    B3D_Collider *collider;
    B3D_Fixed dist;
    B3D_Fixed damage;
    int i;

    b3d_make_projectile_event(&explode_event, B3D_EVENT_EXPLODE, projectile);
    explode_event.position = origin;
    explode_event.damage = projectile->damage;
    b3d_world_push_event(world, &explode_event);

    if (projectile->explosion_radius <= 0) {
        return;
    }

    for (i = 0; i < world->collider_capacity; ++i) {
        collider = &world->colliders[i];
        if (!collider->active) {
            continue;
        }
        if ((collider->layer_mask & projectile->hit_mask) == 0) {
            continue;
        }
        if (b3d_is_owner_safe(projectile, collider->id)) {
            continue;
        }
        if (!b3d_explosion_allowed(world, projectile, collider, origin)) {
            continue;
        }
        if (!b3d_sphere_overlap(origin, projectile->explosion_radius, collider->center, collider->radius)) {
            continue;
        }

        dist = b3d_vec3_distance(origin, collider->center);
        damage = b3d_explosion_damage(projectile, dist);
        if (damage <= 0) {
            continue;
        }

        b3d_hit_clear(&hit);
        hit.hit = B3D_TRUE;
        hit.hit_kind = B3D_HIT_ENTITY;
        hit.target_id = collider->id;
        hit.target_slot = i;
        hit.target_layer = collider->layer_mask;
        hit.target_user_type = collider->user_type;
        hit.point = collider->center;
        hit.normal = b3d_vec3_normalize(b3d_vec3_sub(collider->center, origin));
        hit.user_ptr = collider->user_ptr;
        b3d_emit_damage_event(world, projectile, &hit, damage);
    }
}

static void b3d_handle_hit(B3D_World *world, B3D_Projectile *projectile, const B3D_Hit *hit)
{
    B3D_Vec3 reflected;

    if (hit == 0 || !hit->hit) {
        return;
    }

    projectile->position = hit->point;

    if (hit->hit_kind == B3D_HIT_WORLD) {
        b3d_emit_hit_event(world, B3D_EVENT_HIT_WORLD, projectile, hit);
        if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0) {
            b3d_world_explode_projectile(world, projectile, hit->point);
            b3d_deactivate_projectile(world, projectile);
            return;
        }
        if ((projectile->flags & B3D_PROJ_BOUNCE) != 0 && projectile->bounce > 0) {
            reflected = b3d_vec3_reflect(projectile->velocity, hit->normal);
            projectile->velocity = b3d_vec3_scale(reflected, projectile->bounce);
            projectile->previous_position = projectile->position;
            b3d_emit_hit_event(world, B3D_EVENT_BOUNCE, projectile, hit);
            return;
        }
        if ((projectile->flags & B3D_PROJ_STICK_ON_WORLD) != 0) {
            projectile->velocity = b3d_vec3_zero();
            projectile->acceleration = b3d_vec3_zero();
            projectile->previous_position = projectile->position;
            b3d_emit_hit_event(world, B3D_EVENT_STICK, projectile, hit);
            return;
        }
        b3d_deactivate_projectile(world, projectile);
        return;
    }

    if (hit->hit_kind == B3D_HIT_CUSTOM) {
        b3d_emit_hit_event(world, B3D_EVENT_CONTACT, projectile, hit);
        if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0) {
            b3d_world_explode_projectile(world, projectile, hit->point);
            b3d_deactivate_projectile(world, projectile);
            return;
        }
        if ((projectile->flags & B3D_PROJ_BOUNCE) != 0 && projectile->bounce > 0) {
            reflected = b3d_vec3_reflect(projectile->velocity, hit->normal);
            projectile->velocity = b3d_vec3_scale(reflected, projectile->bounce);
            b3d_emit_hit_event(world, B3D_EVENT_BOUNCE, projectile, hit);
            return;
        }
        if ((projectile->flags & B3D_PROJ_PIERCE) != 0 && projectile->remaining_pierces > 0) {
            projectile->remaining_pierces -= 1;
            projectile->last_hit_id = hit->target_id;
            return;
        }
        b3d_deactivate_projectile(world, projectile);
        return;
    }

    b3d_emit_hit_event(world, B3D_EVENT_HIT_ENTITY, projectile, hit);
    b3d_emit_damage_event(world, projectile, hit, projectile->damage);
    projectile->hit_count += 1;
    projectile->last_hit_id = hit->target_id;

    if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0) {
        b3d_world_explode_projectile(world, projectile, hit->point);
        b3d_deactivate_projectile(world, projectile);
        return;
    }

    if ((projectile->flags & B3D_PROJ_PIERCE) != 0 && projectile->remaining_pierces > 0) {
        projectile->remaining_pierces -= 1;
        if (projectile->max_hits > 0 && projectile->hit_count >= projectile->max_hits) {
            b3d_deactivate_projectile(world, projectile);
        }
        return;
    }

    b3d_deactivate_projectile(world, projectile);
}

void b3d_callbacks_clear(B3D_Callbacks *callbacks)
{
    if (callbacks == 0) {
        return;
    }

    callbacks->user_data = 0;
    callbacks->trace_world = 0;
    callbacks->filter_hit = 0;
    callbacks->filter_explosion = 0;
    callbacks->on_event = 0;
    callbacks->read_transform = 0;
    callbacks->write_transform = 0;
    callbacks->query_collision = 0;
    callbacks->resolve_contact = 0;
    callbacks->query_gravity = 0;
}

void b3d_world_init(B3D_World *world, B3D_Projectile *projectile_buffer, int projectile_capacity, B3D_Event *event_buffer, int event_capacity, B3D_Collider *collider_buffer, int collider_capacity)
{
    int i;

    if (world == 0) {
        return;
    }

    world->projectiles = projectile_buffer;
    world->projectile_capacity = projectile_capacity;
    world->events = event_buffer;
    world->event_capacity = event_capacity;
    world->event_head = 0;
    world->event_tail = 0;
    world->event_count = 0;
    world->event_dropped = 0;
    world->colliders = collider_buffer;
    world->collider_capacity = collider_capacity;
    world->collider_count = 0;
    world->solvers = 0;
    world->solver_capacity = 0;
    world->contact_arena = 0;
    world->contact_capacity = 0;
    b3d_callbacks_clear(&world->callbacks);
    world->next_projectile_id = 1;

    if (world->projectiles != 0) {
        for (i = 0; i < world->projectile_capacity; ++i) {
            b3d_projectile_clear(&world->projectiles[i]);
            world->projectiles[i].slot = i;
        }
    }

    if (world->events != 0) {
        for (i = 0; i < world->event_capacity; ++i) {
            b3d_event_clear(&world->events[i]);
        }
    }

    if (world->colliders != 0) {
        for (i = 0; i < world->collider_capacity; ++i) {
            b3d_collider_clear(&world->colliders[i]);
        }
    }
}

void b3d_world_set_callbacks(B3D_World *world, const B3D_Callbacks *callbacks)
{
    if (world == 0) {
        return;
    }
    if (callbacks == 0) {
        b3d_callbacks_clear(&world->callbacks);
        return;
    }
    world->callbacks = *callbacks;
}

void b3d_world_clear_projectiles(B3D_World *world)
{
    int i;

    if (world == 0 || world->projectiles == 0) {
        return;
    }

    for (i = 0; i < world->projectile_capacity; ++i) {
        b3d_projectile_clear(&world->projectiles[i]);
        world->projectiles[i].slot = i;
        if (world->solvers != 0 && i < world->solver_capacity) {
            b3d_solver_state_clear(&world->solvers[i]);
            world->solvers[i].projectile_slot = i;
        }
    }
}

void b3d_world_clear_colliders(B3D_World *world)
{
    int i;

    if (world == 0 || world->colliders == 0) {
        return;
    }

    for (i = 0; i < world->collider_capacity; ++i) {
        b3d_collider_clear(&world->colliders[i]);
    }
    world->collider_count = 0;
}

int b3d_world_add_collider(B3D_World *world, int id, int layer_mask, B3D_Vec3 center, B3D_Fixed radius, int user_type, void *user_ptr)
{
    int slot;
    B3D_Collider *collider;

    slot = b3d_find_free_collider_slot(world);
    if (slot < 0) {
        return B3D_ID_NONE;
    }

    collider = &world->colliders[slot];
    collider->active = B3D_TRUE;
    collider->id = id;
    collider->layer_mask = layer_mask;
    collider->center = center;
    collider->radius = radius;
    collider->user_type = user_type;
    collider->user_ptr = user_ptr;
    world->collider_count += 1;
    return slot;
}

int b3d_world_remove_collider(B3D_World *world, int id)
{
    int i;

    if (world == 0 || world->colliders == 0) {
        return B3D_FALSE;
    }

    for (i = 0; i < world->collider_capacity; ++i) {
        if (world->colliders[i].active && world->colliders[i].id == id) {
            b3d_collider_clear(&world->colliders[i]);
            if (world->collider_count > 0) {
                world->collider_count -= 1;
            }
            return B3D_TRUE;
        }
    }

    return B3D_FALSE;
}

int b3d_world_update_collider(B3D_World *world, int id, B3D_Vec3 center, B3D_Fixed radius, int layer_mask)
{
    B3D_Collider *collider;

    collider = b3d_world_find_collider(world, id);
    if (collider == 0) {
        return B3D_FALSE;
    }

    collider->center = center;
    collider->radius = radius;
    collider->layer_mask = layer_mask;
    return B3D_TRUE;
}

B3D_Collider *b3d_world_find_collider(B3D_World *world, int id)
{
    int i;

    if (world == 0 || world->colliders == 0) {
        return 0;
    }

    for (i = 0; i < world->collider_capacity; ++i) {
        if (world->colliders[i].active && world->colliders[i].id == id) {
            return &world->colliders[i];
        }
    }

    return 0;
}

int b3d_world_poll_event(B3D_World *world, B3D_Event *out_event)
{
    if (world == 0 || out_event == 0 || world->events == 0 || world->event_capacity <= 0) {
        return B3D_FALSE;
    }
    if (world->event_count <= 0) {
        return B3D_FALSE;
    }

    *out_event = world->events[world->event_tail];
    b3d_event_clear(&world->events[world->event_tail]);
    world->event_tail = (world->event_tail + 1) % world->event_capacity;
    world->event_count -= 1;
    return B3D_TRUE;
}

int b3d_world_push_event(B3D_World *world, const B3D_Event *event_value)
{
    B3D_Event overflow_event;

    if (world == 0 || event_value == 0) {
        return B3D_FALSE;
    }

    if (world->callbacks.on_event != 0) {
        world->callbacks.on_event(world->callbacks.user_data, event_value);
    }

    if (world->events == 0 || world->event_capacity <= 0) {
        return B3D_TRUE;
    }

    if (world->event_count >= world->event_capacity) {
        world->event_dropped += 1;
        if (world->event_capacity > 0) {
            b3d_event_clear(&overflow_event);
            overflow_event.type = B3D_EVENT_QUEUE_OVERFLOW;
            overflow_event.flags = world->event_dropped;
            world->events[world->event_head] = overflow_event;
            world->event_head = (world->event_head + 1) % world->event_capacity;
            world->event_tail = world->event_head;
        }
        return B3D_FALSE;
    }

    world->events[world->event_head] = *event_value;
    world->event_head = (world->event_head + 1) % world->event_capacity;
    world->event_count += 1;
    return B3D_TRUE;
}

int b3d_spawn_projectile(B3D_World *world, const B3D_ProjectileDef *def_value, int owner_id, B3D_Vec3 position, B3D_Vec3 direction)
{
    B3D_Vec3 dir;
    B3D_Vec3 velocity;

    if (def_value == 0) {
        return B3D_ID_NONE;
    }

    if ((def_value->flags & B3D_PROJ_DIR_IS_NORMALIZED) != 0) {
        dir = direction;
    } else {
        dir = b3d_vec3_normalize(direction);
    }

    velocity = b3d_vec3_scale(dir, def_value->speed);
    return b3d_spawn_projectile_velocity(world, def_value, owner_id, position, velocity);
}

int b3d_spawn_projectile_velocity(B3D_World *world, const B3D_ProjectileDef *def_value, int owner_id, B3D_Vec3 position, B3D_Vec3 velocity)
{
    int slot;
    B3D_Projectile *projectile;

    if (world == 0 || def_value == 0) {
        return B3D_ID_NONE;
    }

    slot = b3d_find_free_projectile_slot(world);
    if (slot < 0) {
        return B3D_ID_NONE;
    }

    projectile = &world->projectiles[slot];
    if (world->solvers != 0 && slot < world->solver_capacity) {
        b3d_solver_state_clear(&world->solvers[slot]);
        world->solvers[slot].projectile_slot = slot;
    }
    b3d_projectile_clear(projectile);
    projectile->active = B3D_TRUE;
    projectile->id = world->next_projectile_id;
    world->next_projectile_id += 1;
    if (world->next_projectile_id <= 0) {
        world->next_projectile_id = 1;
    }
    projectile->slot = slot;
    projectile->kind = def_value->kind;
    projectile->flags = def_value->flags;
    projectile->owner_id = owner_id;
    projectile->hit_mask = def_value->hit_mask;
    projectile->damage_type = def_value->damage_type;
    projectile->max_hits = def_value->max_hits;
    projectile->remaining_pierces = def_value->pierce_count;
    projectile->position = position;
    projectile->previous_position = position;
    projectile->velocity = velocity;
    projectile->acceleration = b3d_vec3_zero();
    projectile->radius = def_value->radius;
    projectile->damage = def_value->damage;
    projectile->stun = def_value->stun;
    projectile->knockback = def_value->knockback;
    projectile->lifetime = def_value->lifetime;
    projectile->gravity = def_value->gravity;
    projectile->bounce = def_value->bounce;
    projectile->max_distance = def_value->max_distance;
    projectile->owner_safe_time = def_value->owner_safe_time;
    projectile->explosion_radius = def_value->explosion_radius;
    projectile->explosion_inner_radius = def_value->explosion_inner_radius;
    projectile->explosion_force = def_value->explosion_force;
    projectile->user_ptr = def_value->user_ptr;
    b3d_emit_projectile_event(world, B3D_EVENT_SPAWN, projectile);
    return projectile->id;
}

int b3d_destroy_projectile(B3D_World *world, int projectile_id)
{
    B3D_Projectile *projectile;

    projectile = b3d_world_find_projectile(world, projectile_id);
    if (projectile == 0) {
        return B3D_FALSE;
    }
    b3d_deactivate_projectile(world, projectile);
    return B3D_TRUE;
}

B3D_Projectile *b3d_world_find_projectile(B3D_World *world, int projectile_id)
{
    int i;

    if (world == 0 || world->projectiles == 0) {
        return 0;
    }

    for (i = 0; i < world->projectile_capacity; ++i) {
        if (world->projectiles[i].active && world->projectiles[i].id == projectile_id) {
            return &world->projectiles[i];
        }
    }

    return 0;
}

int b3d_trace_segment(B3D_World *world, const B3D_TraceRequest *request, B3D_Hit *out_hit)
{
    B3D_Hit best;
    B3D_Hit local;
    B3D_Hit world_hit;
    B3D_Collider *collider;
    B3D_FilterRequest filter_request;
    B3D_Fixed radius;
    int i;
    int has_best;
    int world_allowed;

    if (out_hit != 0) {
        b3d_hit_clear(out_hit);
    }
    if (world == 0 || request == 0) {
        return B3D_FALSE;
    }

    b3d_hit_clear(&best);
    has_best = B3D_FALSE;

    if (world->colliders != 0 && (request->flags & B3D_PROJ_DISABLE_ENTITY_HITS) == 0) {
        for (i = 0; i < world->collider_capacity; ++i) {
            collider = &world->colliders[i];
            if (!b3d_candidate_allowed(world, request, collider, B3D_HIT_ENTITY, i)) {
                continue;
            }
            if (collider->id == request->ignore_target_id) {
                continue;
            }
            if (collider->id == request->owner_id && (request->flags & B3D_PROJ_OWNER_SAFE) != 0 && request->age < request->owner_safe_time) {
                continue;
            }
            radius = b3d_fixed_add_sat(request->radius, collider->radius);
            if (b3d_segment_sphere(request->from, request->to, collider->center, radius, &local)) {
                local.hit_kind = B3D_HIT_ENTITY;
                local.target_id = collider->id;
                local.target_slot = i;
                local.target_layer = collider->layer_mask;
                local.target_user_type = collider->user_type;
                local.user_ptr = collider->user_ptr;
                if (!has_best || local.t < best.t) {
                    best = local;
                    has_best = B3D_TRUE;
                }
            }
        }
    }

    if (world->callbacks.trace_world != 0 && (request->flags & B3D_PROJ_DISABLE_WORLD_HITS) == 0) {
        b3d_hit_clear(&world_hit);
        if (world->callbacks.trace_world(world->callbacks.user_data, request, &world_hit)) {
            if (world_hit.hit) {
                if (world_hit.hit_kind == B3D_HIT_NONE) {
                    world_hit.hit_kind = B3D_HIT_WORLD;
                }
                world_allowed = B3D_TRUE;
                if (world->callbacks.filter_hit != 0) {
                    filter_request.projectile_id = request->projectile_id;
                    filter_request.projectile_slot = request->projectile_slot;
                    filter_request.owner_id = request->owner_id;
                    filter_request.target_id = world_hit.target_id;
                    filter_request.target_slot = world_hit.target_slot;
                    filter_request.target_layer = world_hit.target_layer;
                    filter_request.target_user_type = world_hit.target_user_type;
                    filter_request.hit_kind = world_hit.hit_kind;
                    filter_request.hit_mask = request->hit_mask;
                    filter_request.projectile_user_ptr = request->projectile_user_ptr;
                    filter_request.target_user_ptr = world_hit.user_ptr;
                    world_allowed = world->callbacks.filter_hit(world->callbacks.user_data, &filter_request);
                }
                if (world_allowed && (!has_best || world_hit.t < best.t)) {
                    best = world_hit;
                    has_best = B3D_TRUE;
                }
            }
        }
    }

    if (has_best && out_hit != 0) {
        *out_hit = best;
    }

    return has_best;
}

void b3d_world_update(B3D_World *world, B3D_Fixed dt)
{
    B3D_Projectile *projectile;
    B3D_TraceRequest request;
    B3D_Hit hit;
    B3D_Vec3 delta;
    B3D_Vec3 gravity_delta;
    B3D_Fixed traveled;
    int i;

    if (world == 0 || world->projectiles == 0 || dt <= 0) {
        return;
    }

    for (i = 0; i < world->projectile_capacity; ++i) {
        projectile = &world->projectiles[i];
        if (!projectile->active) {
            continue;
        }

        if (world->solvers != 0 && i < world->solver_capacity && world->solvers[i].active && world->solvers[i].projectile_id == projectile->id) {
            b3d_solver_update_projectile(world, projectile, &world->solvers[i], dt);
            continue;
        }

        projectile->previous_position = projectile->position;

        if ((projectile->flags & B3D_PROJ_USE_GRAVITY) != 0) {
            gravity_delta = b3d_vec3(0, b3d_fixed_neg(projectile->gravity), 0);
            gravity_delta = b3d_vec3_scale(gravity_delta, dt);
            projectile->velocity = b3d_vec3_add(projectile->velocity, gravity_delta);
        }

        if (projectile->acceleration.x != 0 || projectile->acceleration.y != 0 || projectile->acceleration.z != 0) {
            projectile->velocity = b3d_vec3_add(projectile->velocity, b3d_vec3_scale(projectile->acceleration, dt));
        }

        delta = b3d_vec3_scale(projectile->velocity, dt);
        projectile->position = b3d_vec3_add(projectile->position, delta);
        projectile->age = b3d_fixed_add_sat(projectile->age, dt);
        traveled = b3d_vec3_length(delta);
        projectile->distance_traveled = b3d_fixed_add_sat(projectile->distance_traveled, traveled);

        request.projectile_id = projectile->id;
        request.projectile_slot = projectile->slot;
        request.owner_id = projectile->owner_id;
        request.hit_mask = projectile->hit_mask;
        request.flags = projectile->flags;
        request.radius = projectile->radius;
        request.age = projectile->age;
        request.owner_safe_time = projectile->owner_safe_time;
        request.from = projectile->previous_position;
        request.to = projectile->position;
        request.ignore_target_id = projectile->last_hit_id;
        request.projectile_user_ptr = projectile->user_ptr;

        b3d_hit_clear(&hit);
        if (b3d_trace_segment(world, &request, &hit)) {
            b3d_handle_hit(world, projectile, &hit);
        }

        if (!projectile->active) {
            continue;
        }

        if ((projectile->flags & B3D_PROJ_EMIT_MOVE) != 0) {
            b3d_emit_projectile_event(world, B3D_EVENT_MOVE, projectile);
        }

        if (projectile->lifetime > 0 && projectile->age >= projectile->lifetime) {
            if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0 && projectile->explosion_radius > 0) {
                b3d_world_explode_projectile(world, projectile, projectile->position);
            }
            b3d_emit_projectile_event(world, B3D_EVENT_EXPIRE, projectile);
            b3d_deactivate_projectile(world, projectile);
            continue;
        }

        if (projectile->max_distance > 0 && projectile->distance_traveled >= projectile->max_distance) {
            b3d_emit_projectile_event(world, B3D_EVENT_EXPIRE, projectile);
            b3d_deactivate_projectile(world, projectile);
            continue;
        }
    }
}

int b3d_fire_hitscan(B3D_World *world, int owner_id, B3D_Vec3 origin, B3D_Vec3 direction, B3D_Fixed range, B3D_Fixed damage, int damage_type, int hit_mask, B3D_Hit *out_hit)
{
    B3D_TraceRequest request;
    B3D_Hit hit;
    B3D_Event event_value;
    B3D_Vec3 dir;
    B3D_Vec3 end;
    int result;

    if (out_hit != 0) {
        b3d_hit_clear(out_hit);
    }
    if (world == 0 || range <= 0) {
        return B3D_FALSE;
    }

    dir = b3d_vec3_normalize(direction);
    end = b3d_vec3_add(origin, b3d_vec3_scale(dir, range));

    request.projectile_id = B3D_ID_NONE;
    request.projectile_slot = B3D_ID_NONE;
    request.owner_id = owner_id;
    request.hit_mask = hit_mask;
    request.flags = B3D_PROJ_DISABLE_ENTITY_HITS & 0;
    request.radius = 0;
    request.age = 0;
    request.owner_safe_time = 0;
    request.from = origin;
    request.to = end;
    request.ignore_target_id = B3D_ID_NONE;
    request.projectile_user_ptr = 0;

    b3d_event_clear(&event_value);
    event_value.type = B3D_EVENT_HITSCAN;
    event_value.owner_id = owner_id;
    event_value.damage = damage;
    event_value.damage_type = damage_type;
    event_value.position = origin;
    event_value.velocity = dir;

    b3d_hit_clear(&hit);
    result = b3d_trace_segment(world, &request, &hit);
    if (result) {
        event_value.target_id = hit.target_id;
        event_value.target_slot = hit.target_slot;
        event_value.target_layer = hit.target_layer;
        event_value.target_user_type = hit.target_user_type;
        event_value.position = hit.point;
        event_value.normal = hit.normal;
        event_value.t = hit.t;
        event_value.target_user_ptr = hit.user_ptr;
        event_value.user_ptr = hit.user_ptr;
        b3d_world_push_event(world, &event_value);

        if (hit.hit_kind == B3D_HIT_ENTITY) {
            event_value.type = B3D_EVENT_DAMAGE;
            b3d_world_push_event(world, &event_value);
        }
        if (out_hit != 0) {
            *out_hit = hit;
        }
        return B3D_TRUE;
    }

    event_value.position = end;
    b3d_world_push_event(world, &event_value);
    return B3D_FALSE;
}

int b3d_sweep_melee(B3D_World *world, int owner_id, B3D_Vec3 from, B3D_Vec3 to, B3D_Fixed radius, B3D_Fixed damage, int damage_type, int hit_mask, B3D_Hit *out_hit)
{
    B3D_TraceRequest request;
    B3D_Hit hit;
    B3D_Event event_value;
    int result;

    if (out_hit != 0) {
        b3d_hit_clear(out_hit);
    }
    if (world == 0) {
        return B3D_FALSE;
    }

    request.projectile_id = B3D_ID_NONE;
    request.projectile_slot = B3D_ID_NONE;
    request.owner_id = owner_id;
    request.hit_mask = hit_mask;
    request.flags = 0;
    request.radius = radius;
    request.age = 0;
    request.owner_safe_time = 0;
    request.from = from;
    request.to = to;
    request.ignore_target_id = B3D_ID_NONE;
    request.projectile_user_ptr = 0;

    b3d_hit_clear(&hit);
    result = b3d_trace_segment(world, &request, &hit);
    if (!result) {
        return B3D_FALSE;
    }

    b3d_event_clear(&event_value);
    event_value.type = B3D_EVENT_HIT_ENTITY;
    if (hit.hit_kind == B3D_HIT_WORLD) {
        event_value.type = B3D_EVENT_HIT_WORLD;
    }
    event_value.owner_id = owner_id;
    event_value.target_id = hit.target_id;
    event_value.target_slot = hit.target_slot;
    event_value.target_layer = hit.target_layer;
    event_value.target_user_type = hit.target_user_type;
    event_value.damage = damage;
    event_value.damage_type = damage_type;
    event_value.t = hit.t;
    event_value.position = hit.point;
    event_value.previous_position = from;
    event_value.normal = hit.normal;
    event_value.velocity = b3d_vec3_sub(to, from);
    event_value.target_user_ptr = hit.user_ptr;
    event_value.user_ptr = hit.user_ptr;
    b3d_world_push_event(world, &event_value);

    if (hit.hit_kind == B3D_HIT_ENTITY) {
        event_value.type = B3D_EVENT_DAMAGE;
        b3d_world_push_event(world, &event_value);
    }

    if (out_hit != 0) {
        *out_hit = hit;
    }
    return B3D_TRUE;
}
