#include "gboltweapon89.h"

#include <string.h>

static B3D_Fixed q12_to_q16(gwp89_fx value)
{
    return (B3D_Fixed)(value * 16L);
}

static gwp89_fx q16_to_q12(B3D_Fixed value)
{
    return (gwp89_fx)(value / 16L);
}

static B3D_Vec3 q12_vec_to_q16(const GWP89_Vec3 *value)
{
    if (!value) return b3d_vec3_zero();
    return b3d_vec3(q12_to_q16(value->x),
                    q12_to_q16(value->y),
                    q12_to_q16(value->z));
}

static GWP89_Vec3 q16_vec_to_q12(B3D_Vec3 value)
{
    GWP89_Vec3 result;
    result.x = q16_to_q12(value.x);
    result.y = q16_to_q12(value.y);
    result.z = q16_to_q12(value.z);
    return result;
}

static int gboltweapon89_trace(void *user_data,
                              const B3D_TraceRequest *request,
                              B3D_Hit *out_hit)
{
    GBoltWeapon89 *bolt;
    GWP89_Vec3 start;
    GWP89_Vec3 delta;
    GBoltSweepHit89 hit;
    bolt = (GBoltWeapon89 *)user_data;
    if (!bolt || !bolt->sweep_fn || !request || !out_hit) return 0;
    start = q16_vec_to_q12(request->from);
    delta.x = q16_to_q12(request->to.x - request->from.x);
    delta.y = q16_to_q12(request->to.y - request->from.y);
    delta.z = q16_to_q12(request->to.z - request->from.z);
    if (!bolt->sweep_fn(bolt->sweep_user, &start, &delta,
                        q16_to_q12(request->radius), &hit))
        return 0;

    b3d_hit_clear(out_hit);
    out_hit->hit = 1;
    out_hit->hit_kind = hit.target_id >= 0
                      ? B3D_HIT_ENTITY : B3D_HIT_WORLD;
    out_hit->target_id = hit.target_id;
    out_hit->target_slot = hit.target_id;
    out_hit->target_layer = hit.target_layer;
    out_hit->target_user_type = hit.material_id;
    out_hit->t = q12_to_q16(hit.fraction_fx);
    out_hit->point = q12_vec_to_q16(&hit.point);
    out_hit->normal = q12_vec_to_q16(&hit.normal);
    return 1;
}

void gboltweapon89_init(GBoltWeapon89 *bolt, GBoltSweep89Fn sweep_fn, void *sweep_user)
{
    B3D_Callbacks callbacks;
    int i;
    if (!bolt) return;
    memset(bolt, 0, sizeof(*bolt));
    bolt->sweep_fn = sweep_fn;
    bolt->sweep_user = sweep_user;
    b3d_world_init(&bolt->world,
                   bolt->projectiles, GBW89_MAX_PROJECTILES,
                   bolt->events, GBW89_MAX_EVENTS,
                   bolt->colliders, GBW89_MAX_COLLIDERS);
    b3d_callbacks_clear(&callbacks);
    callbacks.user_data = bolt;
    callbacks.trace_world = gboltweapon89_trace;
    b3d_world_set_callbacks(&bolt->world, &callbacks);
    for (i = 0; i < GBW89_MAX_PROJECTILES; ++i)
        bolt->drag_q16[i] = 0;
}

int gboltweapon89_spawn(GBoltWeapon89 *bolt,
                       const GWeaponModules89 *modules,
                       int owner_id,
                       const GWP89_Vec3 *origin_q12,
                       const GWP89_Vec3 *direction_q12,
                       gwp89_fx speed_q12,
                       gwp89_fx radius_q12,
                       gwp89_fx damage_q12,
                       unsigned short life_ms,
                       void *user_ptr)
{
    B3D_ProjectileDef def_value;
    B3D_Vec3 origin;
    B3D_Vec3 direction;
    B3D_Projectile *projectile;
    int projectile_id;
    long gravity;
    if (!bolt || !modules || !origin_q12 || !direction_q12) return -1;

    b3d_projectile_def_clear(&def_value);
    def_value.kind = B3D_PROJECTILE_KIND_CUSTOM;
    def_value.flags = B3D_PROJ_USE_GRAVITY | B3D_PROJ_BOUNCE |
                      B3D_PROJ_OWNER_SAFE | B3D_PROJ_EMIT_MOVE |
                      B3D_PROJ_DIR_IS_NORMALIZED;
    def_value.hit_mask = B3D_LAYER_WORLD | B3D_LAYER_ENEMY;
    def_value.damage_type = B3D_DAMAGE_BLUNT;
    def_value.radius = q12_to_q16(radius_q12);
    def_value.speed = q12_to_q16(speed_q12);
    def_value.damage = q12_to_q16(damage_q12);
    def_value.knockback = (B3D_Fixed)((modules->mass_q16 *
                                      (long)def_value.speed) / 65536L);
    def_value.lifetime = (B3D_Fixed)(((unsigned long)life_ms * 65536UL) /
                                     1000UL);
    gravity = modules->gravity_q16;
    if (gravity < 0L) gravity = -gravity;
    def_value.gravity = (B3D_Fixed)gravity;
    def_value.bounce = (B3D_Fixed)modules->bounce_q16;
    def_value.max_distance = q12_to_q16(gwp89_fx_from_int(160));
    def_value.owner_safe_time = 65536L / 10L;
    def_value.max_hits = 1;
    def_value.user_ptr = user_ptr;

    origin = q12_vec_to_q16(origin_q12);
    direction = b3d_vec3_normalize(q12_vec_to_q16(direction_q12));
    projectile_id = b3d_spawn_projectile(&bolt->world, &def_value,
                                         owner_id, origin, direction);
    if (projectile_id < 0) return projectile_id;
    projectile = b3d_world_find_projectile(&bolt->world, projectile_id);
    if (projectile && projectile->slot >= 0 &&
        projectile->slot < GBW89_MAX_PROJECTILES)
        bolt->drag_q16[projectile->slot] = (B3D_Fixed)modules->drag_q16;
    return projectile_id;
}

void gboltweapon89_update(GBoltWeapon89 *bolt, gwp89_fx dt_q12)
{
    int i;
    B3D_Fixed dt_q16;
    B3D_Fixed drag;
    B3D_Fixed factor;
    if (!bolt || dt_q12 <= 0) return;
    dt_q16 = q12_to_q16(dt_q12);
    b3d_world_update(&bolt->world, dt_q16);
    for (i = 0; i < GBW89_MAX_PROJECTILES; ++i) {
        if (!bolt->projectiles[i].active) continue;
        drag = bolt->drag_q16[i];
        if (drag <= 0) continue;
        factor = b3d_fixed_sub_sat(B3D_FIXED_ONE,
                                   b3d_fixed_mul(drag, dt_q16));
        if (factor < 0) factor = 0;
        bolt->projectiles[i].velocity =
            b3d_vec3_scale(bolt->projectiles[i].velocity, factor);
    }
}

const B3D_Projectile *gboltweapon89_get(const GBoltWeapon89 *bolt,
                                      int projectile_id)
{
    int i;
    if (!bolt) return 0;
    for (i = 0; i < GBW89_MAX_PROJECTILES; ++i) {
        if (bolt->projectiles[i].active &&
            bolt->projectiles[i].id == projectile_id)
            return &bolt->projectiles[i];
    }
    return 0;
}

int gboltweapon89_poll_event(GBoltWeapon89 *bolt, B3D_Event *event_out)
{
    if (!bolt || !event_out) return 0;
    return b3d_world_poll_event(&bolt->world, event_out);
}

void gboltweapon89_destroy(GBoltWeapon89 *bolt, int projectile_id)
{
    if (!bolt || projectile_id < 0) return;
    (void)b3d_destroy_projectile(&bolt->world, projectile_id);
}
