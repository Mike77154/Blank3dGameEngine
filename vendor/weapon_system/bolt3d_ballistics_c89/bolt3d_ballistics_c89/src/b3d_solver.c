#include "bolt3d/b3d_world.h"

static B3D_Fixed b3d_solver_shape_radius(const B3D_SolverState *solver, const B3D_Projectile *projectile)
{
    if (solver != 0 && solver->shape.radius > 0) {
        return solver->shape.radius;
    }
    if (projectile != 0) {
        return projectile->radius;
    }
    return 0;
}

static void b3d_solver_make_transform_request(B3D_TransformRequest *request, const B3D_Projectile *projectile, const B3D_SolverState *solver)
{
    request->projectile_id = projectile->id;
    request->projectile_slot = projectile->slot;
    request->owner_id = projectile->owner_id;
    request->binding_id = solver->binding_id;
    request->motion_mode = solver->motion_mode;
    request->flags = solver->flags;
    request->projectile_user_ptr = projectile->user_ptr;
    request->binding_ptr = solver->binding_ptr;
}

static B3D_Vec3 b3d_solver_query_gravity(B3D_World *world, const B3D_Projectile *projectile, const B3D_SolverState *solver, B3D_Fixed dt)
{
    B3D_SolverGravityRequest request;
    B3D_Vec3 gravity_value;

    gravity_value = b3d_vec3(0, b3d_fixed_neg(projectile->gravity), 0);
    if (world->callbacks.query_gravity == 0) {
        return gravity_value;
    }

    request.projectile_id = projectile->id;
    request.projectile_slot = projectile->slot;
    request.owner_id = projectile->owner_id;
    request.binding_id = solver->binding_id;
    request.projectile_flags = projectile->flags;
    request.solver_flags = solver->flags;
    request.age = projectile->age;
    request.dt = dt;
    request.transform = solver->transform;
    request.velocity = projectile->velocity;
    request.acceleration = projectile->acceleration;
    request.fallback_gravity = gravity_value;
    request.projectile_user_ptr = projectile->user_ptr;
    request.binding_ptr = solver->binding_ptr;

    if (world->callbacks.query_gravity(world->callbacks.user_data, &request, &gravity_value)) {
        return gravity_value;
    }
    return request.fallback_gravity;
}

static void b3d_solver_make_event(B3D_Event *event_value, int type_value, const B3D_Projectile *projectile, const B3D_SolverState *solver)
{
    b3d_event_clear(event_value);
    event_value->type = type_value;
    event_value->projectile_id = projectile->id;
    event_value->projectile_slot = projectile->slot;
    event_value->owner_id = projectile->owner_id;
    event_value->damage_type = projectile->damage_type;
    event_value->flags = projectile->flags;
    event_value->motion_mode = solver->motion_mode;
    event_value->response = solver->last_response;
    event_value->iteration = solver->iteration_count;
    event_value->binding_id = solver->binding_id;
    event_value->damage = projectile->damage;
    event_value->stun = projectile->stun;
    event_value->knockback = projectile->knockback;
    event_value->position = solver->solved_transform.position;
    event_value->previous_position = solver->previous_transform.position;
    event_value->velocity = projectile->velocity;
    event_value->previous_transform = solver->previous_transform;
    event_value->desired_transform = solver->desired_transform;
    event_value->solved_transform = solver->solved_transform;
    event_value->source_user_ptr = projectile->user_ptr;
    event_value->user_ptr = projectile->user_ptr;
}

static void b3d_solver_emit_event(B3D_World *world, int type_value, const B3D_Projectile *projectile, const B3D_SolverState *solver)
{
    B3D_Event event_value;

    b3d_solver_make_event(&event_value, type_value, projectile, solver);
    b3d_world_push_event(world, &event_value);
}

static void b3d_solver_emit_transform_event(B3D_World *world, int type_value, const B3D_Projectile *projectile, const B3D_SolverState *solver)
{
    if ((solver->flags & B3D_SOLVER_EMIT_TRANSFORM_EVENTS) == 0) {
        return;
    }
    b3d_solver_emit_event(world, type_value, projectile, solver);
}

static void b3d_solver_emit_contact_event(B3D_World *world, int type_value, const B3D_Projectile *projectile, const B3D_SolverState *solver, const B3D_Contact *contact)
{
    B3D_Event event_value;

    b3d_solver_make_event(&event_value, type_value, projectile, solver);
    event_value.target_id = contact->target_id;
    event_value.target_slot = contact->target_slot;
    event_value.target_layer = contact->target_layer;
    event_value.target_user_type = contact->target_user_type;
    event_value.response = contact->response;
    event_value.t = contact->t;
    event_value.position = contact->point;
    event_value.normal = contact->normal;
    event_value.target_user_ptr = contact->target_user_ptr;
    event_value.user_ptr = contact->target_user_ptr;
    b3d_world_push_event(world, &event_value);
}

static void b3d_solver_emit_damage(B3D_World *world, const B3D_Projectile *projectile, const B3D_SolverState *solver, const B3D_Contact *contact)
{
    B3D_Event event_value;

    b3d_solver_make_event(&event_value, B3D_EVENT_DAMAGE, projectile, solver);
    event_value.target_id = contact->target_id;
    event_value.target_slot = contact->target_slot;
    event_value.target_layer = contact->target_layer;
    event_value.target_user_type = contact->target_user_type;
    event_value.response = contact->response;
    event_value.t = contact->t;
    event_value.position = contact->point;
    event_value.normal = contact->normal;
    event_value.target_user_ptr = contact->target_user_ptr;
    event_value.user_ptr = contact->target_user_ptr;
    b3d_world_push_event(world, &event_value);
}

static int b3d_solver_owner_safe(const B3D_Projectile *projectile, int target_id)
{
    if ((projectile->flags & B3D_PROJ_OWNER_SAFE) == 0) {
        return B3D_FALSE;
    }
    if (projectile->owner_id != target_id) {
        return B3D_FALSE;
    }
    return projectile->age < projectile->owner_safe_time;
}

static int b3d_solver_contact_allowed(B3D_World *world, const B3D_Projectile *projectile, const B3D_Contact *contact)
{
    B3D_FilterRequest request;

    if (contact == 0 || !contact->hit) {
        return B3D_FALSE;
    }
    if ((contact->target_layer & projectile->hit_mask) == 0 && contact->hit_kind == B3D_HIT_ENTITY) {
        return B3D_FALSE;
    }
    if (b3d_solver_owner_safe(projectile, contact->target_id)) {
        return B3D_FALSE;
    }
    if (world->callbacks.filter_hit == 0) {
        return B3D_TRUE;
    }

    request.projectile_id = projectile->id;
    request.projectile_slot = projectile->slot;
    request.owner_id = projectile->owner_id;
    request.target_id = contact->target_id;
    request.target_slot = contact->target_slot;
    request.target_layer = contact->target_layer;
    request.target_user_type = contact->target_user_type;
    request.hit_kind = contact->hit_kind;
    request.hit_mask = projectile->hit_mask;
    request.projectile_user_ptr = projectile->user_ptr;
    request.target_user_ptr = contact->target_user_ptr;
    return world->callbacks.filter_hit(world->callbacks.user_data, &request);
}

static void b3d_solver_contact_from_hit(B3D_Contact *contact, const B3D_Hit *hit)
{
    b3d_contact_clear(contact);
    contact->hit = hit->hit;
    contact->hit_kind = hit->hit_kind;
    contact->target_id = hit->target_id;
    contact->target_slot = hit->target_slot;
    contact->target_layer = hit->target_layer;
    contact->target_user_type = hit->target_user_type;
    contact->t = hit->t;
    contact->point = hit->point;
    contact->normal = hit->normal;
    contact->target_user_ptr = hit->user_ptr;
    if ((contact->target_layer & B3D_LAYER_TRIGGER) != 0) {
        contact->flags |= B3D_CONTACT_TRIGGER;
    }
    if (contact->t <= 0) {
        contact->flags |= B3D_CONTACT_START_INSIDE;
    }
}

static int b3d_solver_query_first(B3D_World *world, B3D_Projectile *projectile, B3D_SolverState *solver, B3D_Vec3 from, B3D_Vec3 to, int ignore_target_id, B3D_Contact *out_contact)
{
    B3D_TraceRequest trace_request;
    B3D_SolverQuery query;
    B3D_Hit hit;
    B3D_Contact candidate;
    B3D_Contact best;
    int has_best;
    int count;
    int i;

    b3d_contact_clear(&best);
    has_best = B3D_FALSE;

    if ((solver->flags & B3D_SOLVER_DISABLE_BUILTIN_QUERY) == 0) {
        trace_request.projectile_id = projectile->id;
        trace_request.projectile_slot = projectile->slot;
        trace_request.owner_id = projectile->owner_id;
        trace_request.hit_mask = projectile->hit_mask;
        trace_request.flags = projectile->flags;
        trace_request.radius = b3d_solver_shape_radius(solver, projectile);
        trace_request.age = projectile->age;
        trace_request.owner_safe_time = projectile->owner_safe_time;
        trace_request.from = from;
        trace_request.to = to;
        trace_request.ignore_target_id = ignore_target_id;
        trace_request.projectile_user_ptr = projectile->user_ptr;
        b3d_hit_clear(&hit);
        if (b3d_trace_segment(world, &trace_request, &hit)) {
            b3d_solver_contact_from_hit(&candidate, &hit);
            best = candidate;
            has_best = B3D_TRUE;
        }
    }

    if ((solver->flags & B3D_SOLVER_DISABLE_CONTRACT_QUERY) == 0 && world->callbacks.query_collision != 0 && world->contact_arena != 0 && world->contact_capacity > 0) {
        query.projectile_id = projectile->id;
        query.projectile_slot = projectile->slot;
        query.owner_id = projectile->owner_id;
        query.binding_id = solver->binding_id;
        query.hit_mask = projectile->hit_mask;
        query.projectile_flags = projectile->flags;
        query.solver_flags = solver->flags;
        query.ignore_target_id = ignore_target_id;
        query.age = projectile->age;
        query.owner_safe_time = projectile->owner_safe_time;
        query.from = solver->transform;
        query.from.position = from;
        query.to = solver->desired_transform;
        query.to.position = to;
        query.shape = solver->shape;
        query.projectile_user_ptr = projectile->user_ptr;
        query.binding_ptr = solver->binding_ptr;

        for (i = 0; i < world->contact_capacity; ++i) {
            b3d_contact_clear(&world->contact_arena[i]);
        }
        count = world->callbacks.query_collision(world->callbacks.user_data, &query, world->contact_arena, world->contact_capacity);
        if (count < 0) {
            count = 0;
        }
        if (count > world->contact_capacity) {
            count = world->contact_capacity;
        }
        for (i = 0; i < count; ++i) {
            candidate = world->contact_arena[i];
            candidate.flags |= B3D_CONTACT_EXTERNAL;
            if (!candidate.hit) {
                continue;
            }
            if (candidate.target_id == ignore_target_id && ignore_target_id != B3D_ID_NONE) {
                continue;
            }
            candidate.t = b3d_fixed_clamp(candidate.t, 0, B3D_FIXED_ONE);
            if ((candidate.target_layer & B3D_LAYER_TRIGGER) != 0) {
                candidate.flags |= B3D_CONTACT_TRIGGER;
            }
            if (candidate.t <= 0) {
                candidate.flags |= B3D_CONTACT_START_INSIDE;
            }
            if (!b3d_solver_contact_allowed(world, projectile, &candidate)) {
                continue;
            }
            if (!has_best || candidate.t < best.t) {
                best = candidate;
                has_best = B3D_TRUE;
            }
        }
    }

    if (has_best && out_contact != 0) {
        *out_contact = best;
    }
    return has_best;
}

static int b3d_solver_default_response(const B3D_Projectile *projectile, const B3D_SolverState *solver, const B3D_Contact *contact)
{
    if ((contact->flags & B3D_CONTACT_TRIGGER) != 0) {
        return B3D_RESPONSE_OVERLAP;
    }
    if ((projectile->flags & B3D_PROJ_PIERCE) != 0 && contact->hit_kind == B3D_HIT_ENTITY && projectile->remaining_pierces > 0) {
        return B3D_RESPONSE_PIERCE;
    }
    if ((projectile->flags & B3D_PROJ_BOUNCE) != 0 && projectile->bounce > 0) {
        return B3D_RESPONSE_BOUNCE;
    }
    if ((projectile->flags & B3D_PROJ_STICK_ON_WORLD) != 0 && contact->hit_kind == B3D_HIT_WORLD) {
        return B3D_RESPONSE_STICK;
    }
    return solver->default_response;
}

static int b3d_solver_resolve_response(B3D_World *world, const B3D_Projectile *projectile, const B3D_SolverState *solver, const B3D_Contact *contact, B3D_Vec3 remaining_delta, int iteration)
{
    B3D_SolverResponseRequest request;
    int response;

    response = b3d_solver_default_response(projectile, solver, contact);
    if (contact->response != B3D_RESPONSE_DEFAULT && b3d_solver_response_is_valid(contact->response)) {
        response = contact->response;
    }
    if (world->callbacks.resolve_contact == 0) {
        return response;
    }

    request.projectile_id = projectile->id;
    request.projectile_slot = projectile->slot;
    request.owner_id = projectile->owner_id;
    request.binding_id = solver->binding_id;
    request.iteration = iteration;
    request.default_response = response;
    request.contact = *contact;
    request.velocity = projectile->velocity;
    request.remaining_delta = remaining_delta;
    request.projectile_user_ptr = projectile->user_ptr;
    request.binding_ptr = solver->binding_ptr;
    response = world->callbacks.resolve_contact(world->callbacks.user_data, &request);
    if (!b3d_solver_response_is_valid(response)) {
        response = request.default_response;
    }
    return response;
}

static B3D_Vec3 b3d_solver_contact_position(B3D_Vec3 point, B3D_Vec3 normal, B3D_Fixed skin)
{
    if (skin <= 0) {
        return point;
    }
    return b3d_vec3_add(point, b3d_vec3_scale(b3d_vec3_normalize(normal), skin));
}

static B3D_Vec3 b3d_solver_advance_past(B3D_Vec3 position, B3D_Vec3 remaining, B3D_Fixed skin, B3D_Vec3 *out_remaining)
{
    B3D_Fixed length;
    B3D_Fixed step;
    B3D_Vec3 advance;

    length = b3d_vec3_length(remaining);
    step = skin;
    if (step <= 0) {
        step = B3D_FIXED_EPSILON;
    }
    if (length <= step) {
        *out_remaining = b3d_vec3_zero();
        return b3d_vec3_add(position, remaining);
    }
    advance = b3d_vec3_scale(b3d_vec3_normalize(remaining), step);
    *out_remaining = b3d_vec3_sub(remaining, advance);
    return b3d_vec3_add(position, advance);
}

static int b3d_solver_process_contact_events(B3D_World *world, B3D_Projectile *projectile, B3D_SolverState *solver, B3D_Contact *contact)
{
    b3d_solver_emit_contact_event(world, B3D_EVENT_CONTACT, projectile, solver, contact);

    if (contact->response == B3D_RESPONSE_OVERLAP || contact->response == B3D_RESPONSE_IGNORE) {
        b3d_solver_emit_contact_event(world, B3D_EVENT_OVERLAP, projectile, solver, contact);
        return B3D_TRUE;
    }

    if (contact->hit_kind == B3D_HIT_WORLD) {
        b3d_solver_emit_contact_event(world, B3D_EVENT_HIT_WORLD, projectile, solver, contact);
    } else if (contact->hit_kind == B3D_HIT_ENTITY) {
        b3d_solver_emit_contact_event(world, B3D_EVENT_HIT_ENTITY, projectile, solver, contact);
        b3d_solver_emit_damage(world, projectile, solver, contact);
        projectile->hit_count += 1;
        projectile->last_hit_id = contact->target_id;
    }

    if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0) {
        b3d_world_explode_projectile(world, projectile, contact->point);
        b3d_destroy_projectile(world, projectile->id);
        return B3D_FALSE;
    }

    if (contact->response == B3D_RESPONSE_PIERCE && projectile->remaining_pierces > 0) {
        projectile->remaining_pierces -= 1;
    }
    if ((solver->flags & B3D_SOLVER_KEEP_ACTIVE_ON_CONTACT) == 0 && projectile->max_hits > 0 && projectile->hit_count >= projectile->max_hits && contact->response != B3D_RESPONSE_OVERLAP) {
        b3d_destroy_projectile(world, projectile->id);
        return B3D_FALSE;
    }
    return B3D_TRUE;
}

void b3d_shape_clear(B3D_Shape *shape)
{
    if (shape == 0) {
        return;
    }
    shape->kind = B3D_SHAPE_POINT;
    shape->user_type = 0;
    shape->radius = 0;
    shape->half_height = 0;
    shape->half_extents = b3d_vec3_zero();
    shape->user_ptr = 0;
}

B3D_Shape b3d_shape_sphere(B3D_Fixed radius)
{
    B3D_Shape shape;

    b3d_shape_clear(&shape);
    shape.kind = B3D_SHAPE_SPHERE;
    shape.radius = radius;
    return shape;
}

void b3d_contact_clear(B3D_Contact *contact)
{
    if (contact == 0) {
        return;
    }
    contact->hit = B3D_FALSE;
    contact->hit_kind = B3D_HIT_NONE;
    contact->target_id = B3D_ID_NONE;
    contact->target_slot = B3D_ID_NONE;
    contact->target_layer = 0;
    contact->target_user_type = 0;
    contact->flags = 0;
    contact->response = B3D_RESPONSE_DEFAULT;
    contact->t = 0;
    contact->point = b3d_vec3_zero();
    contact->normal = b3d_vec3_zero();
    contact->target_user_ptr = 0;
}

void b3d_solver_def_clear(B3D_SolverDef *def_value)
{
    if (def_value == 0) {
        return;
    }
    def_value->motion_mode = B3D_MOTION_SOLVER;
    def_value->flags = B3D_SOLVER_EMIT_TRANSFORM_EVENTS | B3D_SOLVER_KEEP_ACTIVE_ON_CONTACT;
    def_value->default_response = B3D_RESPONSE_SLIDE;
    def_value->max_iterations = B3D_SOLVER_DEFAULT_ITERATIONS;
    def_value->binding_id = B3D_ID_NONE;
    b3d_shape_clear(&def_value->shape);
    def_value->skin = b3d_fixed_div(B3D_FIXED_ONE, b3d_fixed_from_int(256));
    def_value->friction = 0;
    def_value->restitution = B3D_FIXED_ONE;
    def_value->binding_ptr = 0;
}

void b3d_solver_state_clear(B3D_SolverState *solver)
{
    if (solver == 0) {
        return;
    }
    solver->active = B3D_FALSE;
    solver->projectile_id = B3D_ID_NONE;
    solver->projectile_slot = B3D_ID_NONE;
    solver->motion_mode = B3D_MOTION_BALLISTIC;
    solver->flags = 0;
    solver->default_response = B3D_RESPONSE_SLIDE;
    solver->max_iterations = B3D_SOLVER_DEFAULT_ITERATIONS;
    solver->binding_id = B3D_ID_NONE;
    solver->last_target_id = B3D_ID_NONE;
    solver->last_response = B3D_RESPONSE_IGNORE;
    solver->iteration_count = 0;
    b3d_shape_clear(&solver->shape);
    solver->transform = b3d_transform_identity();
    solver->previous_transform = b3d_transform_identity();
    solver->desired_transform = b3d_transform_identity();
    solver->solved_transform = b3d_transform_identity();
    solver->angular_velocity = b3d_vec3_zero();
    solver->skin = 0;
    solver->friction = 0;
    solver->restitution = B3D_FIXED_ONE;
    solver->binding_ptr = 0;
}

int b3d_solver_response_is_valid(int response)
{
    if (response >= B3D_RESPONSE_IGNORE && response <= B3D_RESPONSE_OVERLAP) {
        return B3D_TRUE;
    }
    return response == B3D_RESPONSE_CUSTOM;
}

void b3d_world_attach_solver_arena(B3D_World *world, B3D_SolverState *solver_buffer, int solver_capacity, B3D_Contact *contact_buffer, int contact_capacity)
{
    int i;

    if (world == 0) {
        return;
    }
    world->solvers = solver_buffer;
    world->solver_capacity = solver_capacity;
    world->contact_arena = contact_buffer;
    world->contact_capacity = contact_capacity;

    if (world->solvers != 0) {
        for (i = 0; i < world->solver_capacity; ++i) {
            b3d_solver_state_clear(&world->solvers[i]);
            world->solvers[i].projectile_slot = i;
        }
    }
    if (world->contact_arena != 0) {
        for (i = 0; i < world->contact_capacity; ++i) {
            b3d_contact_clear(&world->contact_arena[i]);
        }
    }
}

B3D_SolverState *b3d_world_find_solver(B3D_World *world, int projectile_id)
{
    B3D_Projectile *projectile;

    if (world == 0 || world->solvers == 0) {
        return 0;
    }
    projectile = b3d_world_find_projectile(world, projectile_id);
    if (projectile == 0 || projectile->slot < 0 || projectile->slot >= world->solver_capacity) {
        return 0;
    }
    if (!world->solvers[projectile->slot].active || world->solvers[projectile->slot].projectile_id != projectile_id) {
        return 0;
    }
    return &world->solvers[projectile->slot];
}

int b3d_enable_projectile_solver(B3D_World *world, int projectile_id, const B3D_SolverDef *solver_def, const B3D_Transform *transform_value)
{
    B3D_Projectile *projectile;
    B3D_SolverState *solver;
    B3D_SolverDef local_def;

    if (world == 0 || world->solvers == 0) {
        return B3D_FALSE;
    }
    projectile = b3d_world_find_projectile(world, projectile_id);
    if (projectile == 0 || projectile->slot < 0 || projectile->slot >= world->solver_capacity) {
        return B3D_FALSE;
    }

    b3d_solver_def_clear(&local_def);
    if (solver_def != 0) {
        local_def = *solver_def;
    }
    solver = &world->solvers[projectile->slot];
    b3d_solver_state_clear(solver);
    solver->active = B3D_TRUE;
    solver->projectile_id = projectile->id;
    solver->projectile_slot = projectile->slot;
    solver->motion_mode = local_def.motion_mode;
    solver->flags = local_def.flags;
    solver->default_response = local_def.default_response;
    solver->max_iterations = local_def.max_iterations;
    if (solver->max_iterations < 1) {
        solver->max_iterations = 1;
    }
    if (solver->max_iterations > B3D_SOLVER_MAX_ITERATIONS) {
        solver->max_iterations = B3D_SOLVER_MAX_ITERATIONS;
    }
    solver->binding_id = local_def.binding_id;
    solver->shape = local_def.shape;
    if (solver->shape.kind == B3D_SHAPE_POINT && projectile->radius > 0) {
        solver->shape = b3d_shape_sphere(projectile->radius);
    }
    solver->skin = local_def.skin;
    solver->friction = b3d_fixed_clamp(local_def.friction, 0, B3D_FIXED_ONE);
    solver->restitution = b3d_fixed_clamp(local_def.restitution, 0, B3D_FIXED_ONE);
    solver->binding_ptr = local_def.binding_ptr;
    if (transform_value != 0) {
        solver->transform = *transform_value;
    } else {
        solver->transform = b3d_transform_from_position(projectile->position);
    }
    solver->previous_transform = solver->transform;
    solver->desired_transform = solver->transform;
    solver->solved_transform = solver->transform;
    projectile->position = solver->transform.position;
    projectile->previous_position = solver->transform.position;
    return B3D_TRUE;
}

int b3d_disable_projectile_solver(B3D_World *world, int projectile_id)
{
    B3D_SolverState *solver;

    solver = b3d_world_find_solver(world, projectile_id);
    if (solver == 0) {
        return B3D_FALSE;
    }
    b3d_solver_state_clear(solver);
    return B3D_TRUE;
}

int b3d_solver_set_desired_transform(B3D_World *world, int projectile_id, const B3D_Transform *transform_value)
{
    B3D_SolverState *solver;

    if (transform_value == 0) {
        return B3D_FALSE;
    }
    solver = b3d_world_find_solver(world, projectile_id);
    if (solver == 0) {
        return B3D_FALSE;
    }
    solver->desired_transform = *transform_value;
    return B3D_TRUE;
}

int b3d_solver_get_transform(B3D_World *world, int projectile_id, B3D_Transform *out_transform)
{
    B3D_SolverState *solver;

    if (out_transform == 0) {
        return B3D_FALSE;
    }
    solver = b3d_world_find_solver(world, projectile_id);
    if (solver == 0) {
        return B3D_FALSE;
    }
    *out_transform = solver->transform;
    return B3D_TRUE;
}

int b3d_spawn_solver(B3D_World *world, const B3D_ProjectileDef *projectile_def, const B3D_SolverDef *solver_def, int owner_id, const B3D_Transform *transform_value, B3D_Vec3 velocity)
{
    B3D_Vec3 position;
    int projectile_id;

    if (transform_value != 0) {
        position = transform_value->position;
    } else {
        position = b3d_vec3_zero();
    }
    projectile_id = b3d_spawn_projectile_velocity(world, projectile_def, owner_id, position, velocity);
    if (projectile_id == B3D_ID_NONE) {
        return B3D_ID_NONE;
    }
    if (!b3d_enable_projectile_solver(world, projectile_id, solver_def, transform_value)) {
        b3d_destroy_projectile(world, projectile_id);
        return B3D_ID_NONE;
    }
    return projectile_id;
}

int b3d_solver_update_projectile(B3D_World *world, B3D_Projectile *projectile, B3D_SolverState *solver, B3D_Fixed dt)
{
    B3D_TransformRequest transform_request;
    B3D_Transform read_transform;
    B3D_Contact contact;
    B3D_Vec3 current_position;
    B3D_Vec3 target_position;
    B3D_Vec3 remaining;
    B3D_Vec3 remaining_after;
    B3D_Vec3 gravity_delta;
    B3D_Vec3 tangent;
    B3D_Fixed remaining_fraction;
    B3D_Fixed keep_factor;
    B3D_Fixed traveled;
    int ignore_target_id;
    int response;
    int iteration;

    if (world == 0 || projectile == 0 || solver == 0 || !projectile->active || !solver->active || dt <= 0) {
        return B3D_FALSE;
    }

    solver->previous_transform = solver->transform;
    solver->last_target_id = B3D_ID_NONE;
    solver->last_response = B3D_RESPONSE_IGNORE;
    solver->iteration_count = 0;
    b3d_solver_emit_transform_event(world, B3D_EVENT_SOLVER_BEGIN, projectile, solver);

    b3d_solver_make_transform_request(&transform_request, projectile, solver);
    if ((solver->flags & B3D_SOLVER_READ_TRANSFORM) != 0 && world->callbacks.read_transform != 0) {
        read_transform = solver->transform;
        if (world->callbacks.read_transform(world->callbacks.user_data, &transform_request, &read_transform)) {
            if (solver->motion_mode == B3D_MOTION_EXTERNAL) {
                solver->desired_transform = read_transform;
            } else {
                solver->transform = read_transform;
                solver->previous_transform = read_transform;
            }
            b3d_solver_emit_transform_event(world, B3D_EVENT_TRANSFORM_READ, projectile, solver);
        }
    }

    projectile->previous_position = solver->transform.position;
    projectile->age = b3d_fixed_add_sat(projectile->age, dt);

    if (solver->motion_mode == B3D_MOTION_SOLVER) {
        if ((projectile->flags & B3D_PROJ_USE_GRAVITY) != 0) {
            gravity_delta = b3d_solver_query_gravity(world, projectile, solver, dt);
            projectile->velocity = b3d_vec3_add(projectile->velocity, b3d_vec3_scale(gravity_delta, dt));
        }
        if (projectile->acceleration.x != 0 || projectile->acceleration.y != 0 || projectile->acceleration.z != 0) {
            projectile->velocity = b3d_vec3_add(projectile->velocity, b3d_vec3_scale(projectile->acceleration, dt));
        }
        solver->desired_transform = solver->transform;
        solver->desired_transform.position = b3d_vec3_add(solver->transform.position, b3d_vec3_scale(projectile->velocity, dt));
        if ((solver->flags & B3D_SOLVER_USE_ANGULAR_VELOCITY) != 0) {
            solver->desired_transform.basis = b3d_basis_integrate_angular(solver->transform.basis, solver->angular_velocity, dt);
        }
        if ((solver->flags & B3D_SOLVER_ALIGN_TO_VELOCITY) != 0 && b3d_vec3_length_sq(projectile->velocity) > B3D_FIXED_EPSILON) {
            solver->desired_transform.basis = b3d_basis_from_forward_up(projectile->velocity, solver->transform.basis.up);
        }
    }

    current_position = solver->transform.position;
    target_position = solver->desired_transform.position;
    remaining = b3d_vec3_sub(target_position, current_position);
    ignore_target_id = B3D_ID_NONE;
    solver->solved_transform = solver->desired_transform;
    solver->solved_transform.position = current_position;

    for (iteration = 0; iteration < solver->max_iterations; ++iteration) {
        solver->iteration_count = iteration + 1;
        if (b3d_vec3_length_sq(remaining) <= B3D_FIXED_EPSILON) {
            break;
        }
        target_position = b3d_vec3_add(current_position, remaining);
        b3d_contact_clear(&contact);
        if (!b3d_solver_query_first(world, projectile, solver, current_position, target_position, ignore_target_id, &contact)) {
            current_position = target_position;
            remaining = b3d_vec3_zero();
            break;
        }

        response = b3d_solver_resolve_response(world, projectile, solver, &contact, remaining, iteration);
        if (!b3d_solver_response_is_valid(response) || response == B3D_RESPONSE_CUSTOM) {
            response = B3D_RESPONSE_BLOCK;
        }
        contact.response = response;
        solver->last_target_id = contact.target_id;
        solver->last_response = response;

        remaining_fraction = b3d_fixed_sub_sat(B3D_FIXED_ONE, contact.t);
        remaining_fraction = b3d_fixed_clamp(remaining_fraction, 0, B3D_FIXED_ONE);
        remaining_after = b3d_vec3_scale(remaining, remaining_fraction);
        current_position = contact.point;
        solver->solved_transform.position = current_position;

        if (!b3d_solver_process_contact_events(world, projectile, solver, &contact)) {
            return B3D_FALSE;
        }
        if (!projectile->active) {
            return B3D_FALSE;
        }

        if (response == B3D_RESPONSE_IGNORE || response == B3D_RESPONSE_OVERLAP || response == B3D_RESPONSE_PIERCE) {
            ignore_target_id = contact.target_id;
            current_position = b3d_solver_advance_past(current_position, remaining_after, solver->skin, &remaining);
            continue;
        }
        if (response == B3D_RESPONSE_SLIDE) {
            current_position = b3d_solver_contact_position(current_position, contact.normal, solver->skin);
            keep_factor = b3d_fixed_sub_sat(B3D_FIXED_ONE, solver->friction);
            tangent = b3d_vec3_reject(remaining_after, contact.normal);
            remaining = b3d_vec3_scale(tangent, keep_factor);
            projectile->velocity = b3d_vec3_scale(b3d_vec3_reject(projectile->velocity, contact.normal), keep_factor);
            b3d_solver_emit_contact_event(world, B3D_EVENT_SLIDE, projectile, solver, &contact);
            ignore_target_id = contact.target_id;
            continue;
        }
        if (response == B3D_RESPONSE_BOUNCE) {
            current_position = b3d_solver_contact_position(current_position, contact.normal, solver->skin);
            remaining = b3d_vec3_scale(b3d_vec3_reflect(remaining_after, contact.normal), solver->restitution);
            projectile->velocity = b3d_vec3_scale(b3d_vec3_reflect(projectile->velocity, contact.normal), solver->restitution);
            b3d_solver_emit_contact_event(world, B3D_EVENT_BOUNCE, projectile, solver, &contact);
            ignore_target_id = contact.target_id;
            continue;
        }
        if (response == B3D_RESPONSE_STICK) {
            current_position = b3d_solver_contact_position(current_position, contact.normal, solver->skin);
            projectile->velocity = b3d_vec3_zero();
            solver->angular_velocity = b3d_vec3_zero();
            remaining = b3d_vec3_zero();
            b3d_solver_emit_contact_event(world, B3D_EVENT_STICK, projectile, solver, &contact);
            break;
        }

        current_position = b3d_solver_contact_position(current_position, contact.normal, solver->skin);
        projectile->velocity = b3d_vec3_zero();
        remaining = b3d_vec3_zero();
        b3d_solver_emit_contact_event(world, B3D_EVENT_BLOCKED, projectile, solver, &contact);
        break;
    }

    if (iteration >= solver->max_iterations && b3d_vec3_length_sq(remaining) > B3D_FIXED_EPSILON) {
        b3d_solver_emit_event(world, B3D_EVENT_SOLVER_FAILED, projectile, solver);
    }

    solver->solved_transform = solver->desired_transform;
    solver->solved_transform.position = current_position;
    solver->transform = solver->solved_transform;
    projectile->position = current_position;
    traveled = b3d_vec3_distance(projectile->previous_position, projectile->position);
    projectile->distance_traveled = b3d_fixed_add_sat(projectile->distance_traveled, traveled);

    b3d_solver_emit_transform_event(world, B3D_EVENT_TRANSFORM_SOLVED, projectile, solver);
    if ((solver->flags & B3D_SOLVER_WRITE_TRANSFORM) != 0 && world->callbacks.write_transform != 0) {
        if (world->callbacks.write_transform(world->callbacks.user_data, &transform_request, &solver->transform)) {
            b3d_solver_emit_transform_event(world, B3D_EVENT_TRANSFORM_WRITE, projectile, solver);
        }
    }

    if ((projectile->flags & B3D_PROJ_EMIT_MOVE) != 0) {
        b3d_solver_emit_event(world, B3D_EVENT_MOVE, projectile, solver);
    }

    if (projectile->lifetime > 0 && projectile->age >= projectile->lifetime) {
        if ((projectile->flags & B3D_PROJ_EXPLODE_ON_HIT) != 0 && projectile->explosion_radius > 0) {
            b3d_world_explode_projectile(world, projectile, projectile->position);
        }
        b3d_solver_emit_event(world, B3D_EVENT_EXPIRE, projectile, solver);
        b3d_destroy_projectile(world, projectile->id);
        return B3D_FALSE;
    }
    if (projectile->max_distance > 0 && projectile->distance_traveled >= projectile->max_distance) {
        b3d_solver_emit_event(world, B3D_EVENT_EXPIRE, projectile, solver);
        b3d_destroy_projectile(world, projectile->id);
        return B3D_FALSE;
    }
    return B3D_TRUE;
}
