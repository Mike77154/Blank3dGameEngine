#include "blank3d_vertical_axis.h"

#include <string.h>

static vm89_scalar b3d_vm_add(void *user, vm89_scalar a, vm89_scalar b)
{
    (void)user;
    return (vm89_scalar)g3d_fix_add_sat((g3d_fix)a, (g3d_fix)b);
}

static vm89_scalar b3d_vm_sub(void *user, vm89_scalar a, vm89_scalar b)
{
    (void)user;
    return (vm89_scalar)g3d_fix_sub_sat((g3d_fix)a, (g3d_fix)b);
}

static vm89_scalar b3d_vm_mul(void *user, vm89_scalar a, vm89_scalar b)
{
    (void)user;
    return (vm89_scalar)g3d_fix_mul((g3d_fix)a, (g3d_fix)b);
}

static vm89_scalar b3d_vm_abs(void *user, vm89_scalar value)
{
    (void)user;
    if (value < 0) return (vm89_scalar)g3d_fix_neg_sat((g3d_fix)value);
    return value;
}

static vm89_scalar b3d_vm_length3(void *user, const vm89_vec3 *value)
{
    Vec3 converted;
    (void)user;
    if (!value) return 0;
    converted = gamlib_vec3((g3d_fix)value->x,
                            (g3d_fix)value->y,
                            (g3d_fix)value->z);
    return (vm89_scalar)gamlib_vec3_length(&converted);
}

static int b3d_vm_normalize3(void *user, const vm89_vec3 *value,
                             vm89_vec3 *out_normalized)
{
    Vec3 converted;
    Vec3 normalized;
    (void)user;
    if (!value || !out_normalized) return 0;
    converted = gamlib_vec3((g3d_fix)value->x,
                            (g3d_fix)value->y,
                            (g3d_fix)value->z);
    if (gamlib_vec3_length(&converted) <= G3D_FIX_EPSILON) return 0;
    gamlib_vec3_normalize(&normalized, &converted);
    out_normalized->x = (vm89_scalar)normalized.x;
    out_normalized->y = (vm89_scalar)normalized.y;
    out_normalized->z = (vm89_scalar)normalized.z;
    return 1;
}

static int b3d_vm_get_position(void *user, void *actor,
                               vm89_vec3 *out_position)
{
    Transform *transform;
    (void)user;
    if (!actor || !out_position) return 0;
    transform = (Transform *)actor;
    out_position->x = (vm89_scalar)transform->position.x;
    out_position->y = (vm89_scalar)transform->position.y;
    out_position->z = (vm89_scalar)transform->position.z;
    return 1;
}

static int b3d_vm_set_position(void *user, void *actor,
                               const vm89_vec3 *position)
{
    Transform *transform;
    (void)user;
    if (!actor || !position) return 0;
    transform = (Transform *)actor;
    transform->position.x = (g3d_fix)position->x;
    transform->position.y = (g3d_fix)position->y;
    transform->position.z = (g3d_fix)position->z;
    return 1;
}

static vm89_vec3 b3d_vm_from_vec3(const Vec3 *value)
{
    vm89_vec3 result;
    result.x = value ? (vm89_scalar)value->x : 0;
    result.y = value ? (vm89_scalar)value->y : 0;
    result.z = value ? (vm89_scalar)value->z : 0;
    return result;
}

static Vec3 b3d_vec3_from_vm(const vm89_vec3 *value)
{
    if (!value) return gamlib_vec3(0, 0, 0);
    return gamlib_vec3((g3d_fix)value->x,
                       (g3d_fix)value->y,
                       (g3d_fix)value->z);
}

static void b3d_vertical_rebind(Blank3DVerticalAxis *axis)
{
    if (!axis) return;
    fly89_init(&axis->fly, &axis->providers);
    jump89_init(&axis->jump, &axis->providers);
    airdiver89_init(&axis->airdiver, &axis->providers);
}

void blank3d_vertical_axis_init_gamlib3d(Blank3DVerticalAxis *axis)
{
    if (!axis) return;
    memset(axis, 0, sizeof(*axis));
    axis->providers.math.user = 0;
    axis->providers.math.one = G3D_FIX_ONE;
    axis->providers.math.epsilon = G3D_FIX_EPSILON;
    axis->providers.math.add = b3d_vm_add;
    axis->providers.math.sub = b3d_vm_sub;
    axis->providers.math.mul = b3d_vm_mul;
    axis->providers.math.abs_value = b3d_vm_abs;
    axis->providers.math.length3 = b3d_vm_length3;
    axis->providers.math.normalize3 = b3d_vm_normalize3;
    axis->providers.transform.user = 0;
    axis->providers.transform.get_position = b3d_vm_get_position;
    axis->providers.transform.set_position = b3d_vm_set_position;
    axis->gravity_enabled = 1;
    axis->gravity_fall_speed = G3D_FIX_FROM_INT(9);
    axis->jump_gravity = G3D_FIX_FROM_INT(22);
    axis->initialized = 1;
    b3d_vertical_rebind(axis);
}

void blank3d_vertical_axis_set_providers(
    Blank3DVerticalAxis *axis,
    const vm89_provider_bundle *providers)
{
    if (!axis || !providers) return;
    axis->providers = *providers;
    axis->initialized = 1;
    b3d_vertical_rebind(axis);
}

void blank3d_vertical_axis_set_physics_provider(
    Blank3DVerticalAxis *axis,
    const vm89_physics_provider *physics_provider)
{
    if (!axis) return;
    memset(&axis->providers.physics, 0, sizeof(axis->providers.physics));
    if (physics_provider) axis->providers.physics = *physics_provider;
    b3d_vertical_rebind(axis);
}

void blank3d_vertical_axis_configure_gravity(
    Blank3DVerticalAxis *axis,
    int enabled,
    g3d_fix fall_speed,
    g3d_fix jump_gravity)
{
    if (!axis) return;
    axis->gravity_enabled = enabled != 0;
    if (fall_speed < 0) fall_speed = g3d_fix_neg_sat(fall_speed);
    if (jump_gravity < 0) jump_gravity = g3d_fix_neg_sat(jump_gravity);
    axis->gravity_fall_speed = fall_speed;
    axis->jump_gravity = jump_gravity;
}

void blank3d_vertical_body_init(Blank3DVerticalBody *body,
                                void *actor,
                                g3d_fix spawn_y,
                                g3d_fix floor_y)
{
    if (!body) return;
    memset(body, 0, sizeof(*body));
    body->actor = actor;
    body->spawn_y = spawn_y;
    body->floor_y = floor_y;
    body->born_above_floor = spawn_y > floor_y;
    jump89_state_reset(&body->jump_state);
    airdiver89_state_reset(&body->airdiver_state);
}

void blank3d_vertical_body_set_flying(Blank3DVerticalBody *body,
                                      int flying_entity)
{
    if (!body) return;
    body->flying_entity = flying_entity != 0;
}

void blank3d_vertical_body_set_gravity_immune(Blank3DVerticalBody *body,
                                               int gravity_immune)
{
    if (!body) return;
    body->gravity_immune = gravity_immune != 0;
}

void blank3d_vertical_body_set_floor(Blank3DVerticalBody *body,
                                     g3d_fix floor_y)
{
    if (!body) return;
    body->floor_y = floor_y;
    body->born_above_floor = body->spawn_y > floor_y;
}

void blank3d_vertical_axis_tick(Blank3DVerticalAxis *axis,
                                Blank3DVerticalBody *body,
                                g3d_fix dt)
{
    if (!axis || !axis->initialized || !body || !body->actor || dt <= 0)
        return;
    if (jump89_is_active(&body->jump_state)) {
        (void)jump89_tick(&axis->jump, &body->jump_state,
                          body->actor, axis->jump_gravity, dt,
                          body->floor_y, axis->gravity_enabled);
        return;
    }
    if (!axis->gravity_enabled || body->flying_entity ||
        body->gravity_immune) return;
    (void)fly89_move_vertical(&axis->fly, body->actor,
                              axis->gravity_fall_speed, dt,
                              FLY89_DOWN, body->floor_y);
}

int blank3d_vertical_axis_fly(Blank3DVerticalAxis *axis,
                             Blank3DVerticalBody *body,
                             g3d_fix speed,
                             g3d_fix dt,
                             int direction)
{
    if (!axis || !body || !body->actor) return 0;
    return fly89_move_vertical(&axis->fly, body->actor, speed, dt,
                               direction, body->floor_y);
}

int blank3d_vertical_axis_jump(Blank3DVerticalAxis *axis,
                              Blank3DVerticalBody *body,
                              g3d_fix impulse)
{
    if (!axis || !body || !body->actor) return 0;
    return jump89_start(&axis->jump, &body->jump_state, body->actor,
                        impulse, body->floor_y, axis->gravity_enabled);
}

int blank3d_vertical_axis_grounded(Blank3DVerticalAxis *axis,
                                  Blank3DVerticalBody *body)
{
    if (!axis || !body || !body->actor) return 0;
    return fly89_grounded(&axis->fly, body->actor, body->floor_y);
}

g3d_fix blank3d_vertical_axis_height(Blank3DVerticalAxis *axis,
                                     Blank3DVerticalBody *body)
{
    if (!axis || !body || !body->actor) return 0;
    return (g3d_fix)fly89_height(&axis->fly, body->actor, body->floor_y);
}

int blank3d_vertical_axis_jumping(const Blank3DVerticalBody *body)
{
    return body && jump89_is_active(&body->jump_state);
}

int blank3d_vertical_axis_falling(const Blank3DVerticalBody *body)
{
    return body && jump89_is_falling(&body->jump_state);
}

int blank3d_vertical_axis_save_position(Blank3DVerticalAxis *axis,
                                        Blank3DVerticalBody *body)
{
    if (!axis || !body || !body->actor) return 0;
    return airdiver89_save_position(&axis->airdiver,
                                    &body->airdiver_state,
                                    body->actor);
}

int blank3d_vertical_axis_descend_to_y(Blank3DVerticalAxis *axis,
                                       Blank3DVerticalBody *body,
                                       g3d_fix target_y,
                                       g3d_fix speed,
                                       g3d_fix dt,
                                       int *out_reached)
{
    if (!axis || !body || !body->actor) return 0;
    return airdiver89_descend_to_y(&axis->airdiver, body->actor,
                                   target_y, speed, dt, body->floor_y,
                                   out_reached);
}

int blank3d_vertical_axis_ram_point(Blank3DVerticalAxis *axis,
                                    Blank3DVerticalBody *body,
                                    const Vec3 *target,
                                    g3d_fix speed,
                                    g3d_fix dt,
                                    int *out_reached)
{
    vm89_vec3 converted;
    if (!axis || !body || !body->actor || !target) return 0;
    converted = b3d_vm_from_vec3(target);
    return airdiver89_ram_target(&axis->airdiver, body->actor,
                                 &converted, speed, dt, body->floor_y,
                                 out_reached);
}

int blank3d_vertical_axis_return_saved(Blank3DVerticalAxis *axis,
                                       Blank3DVerticalBody *body,
                                       g3d_fix speed,
                                       g3d_fix dt,
                                       int *out_reached)
{
    if (!axis || !body || !body->actor) return 0;
    return airdiver89_return_saved(&axis->airdiver,
                                   &body->airdiver_state,
                                   body->actor, speed, dt,
                                   body->floor_y, out_reached);
}

int blank3d_vertical_axis_saved_position(
    const Blank3DVerticalBody *body,
    Vec3 *out_position)
{
    vm89_vec3 saved;
    if (!body || !out_position ||
        !airdiver89_saved_position(&body->airdiver_state, &saved)) return 0;
    *out_position = b3d_vec3_from_vm(&saved);
    return 1;
}
