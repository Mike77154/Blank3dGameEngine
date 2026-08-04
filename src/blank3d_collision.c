#include "blank3d_collision.h"

#include <string.h>

#define B3D_FAR_AWAY_Q12 ((gwp89_fx)(1000L * GWP89_FIX_ONE))

static void b3d_collision_status(Blank3DCollision *collision, const char *text)
{
    if (!collision) return;
    if (!text) text = "";
    strncpy(collision->status, text, sizeof(collision->status) - 1U);
    collision->status[sizeof(collision->status) - 1U] = '\0';
}

static ccs_fixed b3d_q12_to_ccs(gwp89_fx value)
{
    long scaled;
    scaled = value * 16L;
    if (scaled > 2147483647L) scaled = 2147483647L;
    if (scaled < (-2147483647L - 1L)) scaled = (-2147483647L - 1L);
    return (ccs_fixed)scaled;
}

static gwp89_fx b3d_ccs_to_q12(ccs_fixed value)
{
    return (gwp89_fx)(value / 16);
}

static fx b3d_q12_to_sicol(gwp89_fx value)
{
    if (value > 2147483647L) value = 2147483647L;
    if (value < (-2147483647L - 1L)) value = (-2147483647L - 1L);
    return (fx)value;
}

static gwp89_fx b3d_sicol_to_q12(fx value)
{
    return (gwp89_fx)value;
}

static ccs_vec3 b3d_ccs_vec(const GWP89_Vec3 *value)
{
    ccs_vec3 result;
    result.x = b3d_q12_to_ccs(value ? value->x : 0);
    result.y = b3d_q12_to_ccs(value ? value->y : 0);
    result.z = b3d_q12_to_ccs(value ? value->z : 0);
    return result;
}

static GWP89_Vec3 b3d_gwp_from_ccs(ccs_vec3 value)
{
    GWP89_Vec3 result;
    result.x = b3d_ccs_to_q12(value.x);
    result.y = b3d_ccs_to_q12(value.y);
    result.z = b3d_ccs_to_q12(value.z);
    return result;
}

static GWP89_Vec3 b3d_gwp_from_sicol(const fx value[3])
{
    GWP89_Vec3 result;
    result.x = b3d_sicol_to_q12(value[0]);
    result.y = b3d_sicol_to_q12(value[1]);
    result.z = b3d_sicol_to_q12(value[2]);
    return result;
}

static void b3d_sicol_vec(const GWP89_Vec3 *value, fx out[3])
{
    out[0] = b3d_q12_to_sicol(value ? value->x : 0);
    out[1] = b3d_q12_to_sicol(value ? value->y : 0);
    out[2] = b3d_q12_to_sicol(value ? value->z : 0);
}

static int b3d_enemy_from_ccs(const Blank3DCollision *collision, int body_id)
{
    int i;
    if (!collision) return -1;
    for (i = 0; i < B3D_COLLISION_MAX_ENEMIES; ++i) {
        if (collision->ccs_enemy_ids[i] == body_id) return i;
    }
    return -1;
}

static int b3d_enemy_from_sicol(const Blank3DCollision *collision, int body_id)
{
    int i;
    if (!collision) return -1;
    for (i = 0; i < B3D_COLLISION_MAX_ENEMIES; ++i) {
        if (collision->sicol_enemy_ids[i] == body_id) return i;
    }
    return -1;
}

static int b3d_is_ccs_static(const Blank3DCollision *collision, int body_id)
{
    int i;
    if (!collision) return 0;
    for (i = 0; i < B3D_COLLISION_STATIC_COUNT; ++i) {
        if (collision->ccs_static_ids[i] == body_id) return 1;
    }
    return 0;
}

static int b3d_is_sicol_static(const Blank3DCollision *collision, int body_id)
{
    int i;
    if (!collision) return 0;
    for (i = 0; i < B3D_COLLISION_STATIC_COUNT; ++i) {
        if (collision->sicol_static_ids[i] == body_id) return 1;
    }
    return 0;
}

static gwp89_fx b3d_gwp_length(const GWP89_Vec3 *value)
{
    ccs_vec3 v;
    ccs_fixed length;
    v = b3d_ccs_vec(value);
    length = ccs_vec3_len(v);
    return b3d_ccs_to_q12(length);
}

static void b3d_init_static_ccs(Blank3DCollision *collision,
                                int index,
                                gwp89_fx x,
                                gwp89_fx y,
                                gwp89_fx z,
                                gwp89_fx hx,
                                gwp89_fx hy,
                                gwp89_fx hz)
{
    ccs_shape_box *shape;
    int id;
    shape = &collision->ccs_static_shapes[index];
    memset(shape, 0, sizeof(*shape));
    collision->ccs_static_positions[index] = ccs_vec3_make(
        b3d_q12_to_ccs(x), b3d_q12_to_ccs(y), b3d_q12_to_ccs(z));
    shape->header.type = CCS_SHAPE_BOX;
    shape->header.flags = CCS_SHAPE_FLAG_STATIC;
    shape->center = collision->ccs_static_positions[index];
    shape->half_extents = ccs_vec3_make(
        b3d_q12_to_ccs(hx), b3d_q12_to_ccs(hy), b3d_q12_to_ccs(hz));
    id = ccs_world_add(&collision->ccs_static_positions[index], (ccs_shape *)shape);
    collision->ccs_static_ids[index] = id;
    if (id >= 0) {
        ccs_bodies[id].layer = B3D_COLLISION_LAYER_WORLD;
        ccs_bodies[id].mask = B3D_COLLISION_LAYER_BULLET |
                              B3D_COLLISION_LAYER_PLAYER;
    }
}

static void b3d_init_static_sicol(Blank3DCollision *collision,
                                  int index,
                                  gwp89_fx x,
                                  gwp89_fx y,
                                  gwp89_fx z,
                                  gwp89_fx hx,
                                  gwp89_fx hy,
                                  gwp89_fx hz)
{
    sicol_shape_t shape;
    fx pos[3];
    fx half[3];
    pos[0] = b3d_q12_to_sicol(x);
    pos[1] = b3d_q12_to_sicol(y);
    pos[2] = b3d_q12_to_sicol(z);
    half[0] = b3d_q12_to_sicol(hx);
    half[1] = b3d_q12_to_sicol(hy);
    half[2] = b3d_q12_to_sicol(hz);
    sicol_shape_make_aabb(&shape, pos, half);
    collision->sicol_static_ids[index] = sicol_world_create(
        &collision->sicol_world, &shape,
        B3D_COLLISION_LAYER_WORLD,
        B3D_COLLISION_LAYER_BULLET | B3D_COLLISION_LAYER_PLAYER);
}

void blank3d_collision_init(Blank3DCollision *collision)
{
    int i;
    ccs_shape_capsule *enemy_shape;
    sicol_shape_t sicol_shape;
    fx pos[3];
    fx axis[3];
    if (!collision) return;
    memset(collision, 0, sizeof(*collision));
    ccs_world_clear();
    ccs_world_set_broadphase(CCS_WORLDBP_SWEEP);
    sicol_world_init(&collision->sicol_world);

    for (i = 0; i < B3D_COLLISION_MAX_ENEMIES; ++i) {
        collision->ccs_enemy_positions[i] = ccs_vec3_make(
            b3d_q12_to_ccs(B3D_FAR_AWAY_Q12),
            b3d_q12_to_ccs(B3D_FAR_AWAY_Q12),
            b3d_q12_to_ccs(B3D_FAR_AWAY_Q12));
        enemy_shape = &collision->ccs_enemy_shapes[i];
        memset(enemy_shape, 0, sizeof(*enemy_shape));
        enemy_shape->header.type = CCS_SHAPE_CAPSULE;
        enemy_shape->header.flags = CCS_SHAPE_FLAG_TRIGGER;
        enemy_shape->center = collision->ccs_enemy_positions[i];
        enemy_shape->axis = ccs_vec3_make(0, CCS_FIXED_ONE, 0);
        enemy_shape->half_height = b3d_q12_to_ccs((gwp89_fx)(9L * GWP89_FIX_ONE / 10L));
        enemy_shape->radius = b3d_q12_to_ccs((gwp89_fx)(13L * GWP89_FIX_ONE / 20L));
        collision->ccs_enemy_ids[i] = ccs_world_add(
            &collision->ccs_enemy_positions[i], (ccs_shape *)enemy_shape);
        if (collision->ccs_enemy_ids[i] >= 0) {
            ccs_bodies[collision->ccs_enemy_ids[i]].layer = 0U;
            ccs_bodies[collision->ccs_enemy_ids[i]].mask = 0U;
        }

        pos[0] = b3d_q12_to_sicol(B3D_FAR_AWAY_Q12);
        pos[1] = b3d_q12_to_sicol(B3D_FAR_AWAY_Q12);
        pos[2] = b3d_q12_to_sicol(B3D_FAR_AWAY_Q12);
        axis[0] = 0;
        axis[1] = FX_ONE;
        axis[2] = 0;
        sicol_shape_make_capsule(&sicol_shape, pos, axis,
            (fx)(9L * FX_ONE / 10L), (fx)(13L * FX_ONE / 20L));
        collision->sicol_enemy_ids[i] = sicol_world_create(
            &collision->sicol_world, &sicol_shape, 0U, 0U);
    }

    /* Ground catches downward projectiles; backstop catches long-range rounds. */
    b3d_init_static_ccs(collision, 0,
        0, (gwp89_fx)(-GWP89_FIX_ONE), (gwp89_fx)(20L * GWP89_FIX_ONE),
        (gwp89_fx)(40L * GWP89_FIX_ONE), GWP89_FIX_ONE,
        (gwp89_fx)(40L * GWP89_FIX_ONE));
    b3d_init_static_ccs(collision, 1,
        0, (gwp89_fx)(5L * GWP89_FIX_ONE), (gwp89_fx)(60L * GWP89_FIX_ONE),
        (gwp89_fx)(40L * GWP89_FIX_ONE), (gwp89_fx)(5L * GWP89_FIX_ONE),
        GWP89_FIX_ONE);
    b3d_init_static_sicol(collision, 0,
        0, (gwp89_fx)(-GWP89_FIX_ONE), (gwp89_fx)(20L * GWP89_FIX_ONE),
        (gwp89_fx)(40L * GWP89_FIX_ONE), GWP89_FIX_ONE,
        (gwp89_fx)(40L * GWP89_FIX_ONE));
    b3d_init_static_sicol(collision, 1,
        0, (gwp89_fx)(5L * GWP89_FIX_ONE), (gwp89_fx)(60L * GWP89_FIX_ONE),
        (gwp89_fx)(40L * GWP89_FIX_ONE), (gwp89_fx)(5L * GWP89_FIX_ONE),
        GWP89_FIX_ONE);

    collision->initialized = 1;
    b3d_collision_status(collision,
        "collision online: CCS ray/broadphase + SICOL-DE swept sphere");
}

void blank3d_collision_set_enemy(Blank3DCollision *collision,
                                 int enemy_index,
                                 int active,
                                 gwp89_fx x,
                                 gwp89_fx y,
                                 gwp89_fx z)
{
    int ccs_id;
    int sicol_id;
    fx pos[3];
    gwp89_fx center_y;
    if (!collision || !collision->initialized) return;
    if (enemy_index < 0 || enemy_index >= B3D_COLLISION_MAX_ENEMIES) return;
    center_y = active ? y + (gwp89_fx)(9L * GWP89_FIX_ONE / 10L)
                      : B3D_FAR_AWAY_Q12;
    if (!active) {
        x = B3D_FAR_AWAY_Q12;
        z = B3D_FAR_AWAY_Q12;
    }

    collision->ccs_enemy_positions[enemy_index] = ccs_vec3_make(
        b3d_q12_to_ccs(x), b3d_q12_to_ccs(center_y), b3d_q12_to_ccs(z));
    ccs_id = collision->ccs_enemy_ids[enemy_index];
    if (ccs_id >= 0) {
        ccs_bodies[ccs_id].layer = active ? B3D_COLLISION_LAYER_ENEMY : 0U;
        ccs_bodies[ccs_id].mask = active
            ? (B3D_COLLISION_LAYER_BULLET | B3D_COLLISION_LAYER_PLAYER)
            : 0U;
    }

    pos[0] = b3d_q12_to_sicol(x);
    pos[1] = b3d_q12_to_sicol(center_y);
    pos[2] = b3d_q12_to_sicol(z);
    sicol_id = collision->sicol_enemy_ids[enemy_index];
    if (sicol_id >= 0) {
        (void)sicol_world_set_position(&collision->sicol_world, sicol_id, pos);
        (void)sicol_world_set_layer_mask(&collision->sicol_world, sicol_id,
            active ? B3D_COLLISION_LAYER_ENEMY : 0U,
            active ? (B3D_COLLISION_LAYER_BULLET | B3D_COLLISION_LAYER_PLAYER) : 0U);
    }
}

void blank3d_collision_step(Blank3DCollision *collision)
{
    if (!collision || !collision->initialized) return;
    ccs_world_step();
}

static int b3d_fill_ccs_hit(Blank3DCollision *collision,
                            const ccs_raycast_hit *hit,
                            int body_id,
                            gwp89_fx path_length,
                            Blank3DCollisionHit *out_hit)
{
    int enemy_index;
    if (!collision || !hit || !out_hit) return 0;
    enemy_index = b3d_enemy_from_ccs(collision, body_id);
    if (enemy_index < 0 && !b3d_is_ccs_static(collision, body_id)) return 0;
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->hit = 1;
    out_hit->enemy_index = enemy_index;
    out_hit->material_id = enemy_index >= 0
        ? B3D_COLLISION_MATERIAL_FLESH : B3D_COLLISION_MATERIAL_WORLD;
    out_hit->distance_fx = b3d_ccs_to_q12(hit->t);
    if (path_length > 0)
        out_hit->fraction_fx = (gwp89_fx)((out_hit->distance_fx * GWP89_FIX_ONE) / path_length);
    else
        out_hit->fraction_fx = 0;
    if (out_hit->fraction_fx < 0) out_hit->fraction_fx = 0;
    if (out_hit->fraction_fx > GWP89_FIX_ONE) out_hit->fraction_fx = GWP89_FIX_ONE;
    out_hit->point = b3d_gwp_from_ccs(hit->point);
    out_hit->normal = b3d_gwp_from_ccs(hit->normal);
    return 1;
}

static int b3d_fill_sicol_ray_hit(Blank3DCollision *collision,
                                  const sicol_world_raycast_hit_t *hit,
                                  gwp89_fx path_length,
                                  Blank3DCollisionHit *out_hit)
{
    int enemy_index;
    if (!collision || !hit || !out_hit) return 0;
    enemy_index = b3d_enemy_from_sicol(collision, hit->id);
    if (enemy_index < 0 && !b3d_is_sicol_static(collision, hit->id)) return 0;
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->hit = 1;
    out_hit->enemy_index = enemy_index;
    out_hit->material_id = enemy_index >= 0
        ? B3D_COLLISION_MATERIAL_FLESH : B3D_COLLISION_MATERIAL_WORLD;
    out_hit->distance_fx = b3d_sicol_to_q12(hit->hit.t);
    if (path_length > 0)
        out_hit->fraction_fx = (gwp89_fx)((out_hit->distance_fx * GWP89_FIX_ONE) / path_length);
    else
        out_hit->fraction_fx = 0;
    if (out_hit->fraction_fx < 0) out_hit->fraction_fx = 0;
    if (out_hit->fraction_fx > GWP89_FIX_ONE) out_hit->fraction_fx = GWP89_FIX_ONE;
    out_hit->point = b3d_gwp_from_sicol(hit->hit.point);
    out_hit->normal = b3d_gwp_from_sicol(hit->hit.normal);
    return 1;
}

int blank3d_collision_raycast(Blank3DCollision *collision,
                              const GWP89_Vec3 *origin,
                              const GWP89_Vec3 *direction,
                              gwp89_fx range_fx,
                              unsigned int layer_mask,
                              Blank3DCollisionHit *out_hit)
{
    ccs_ray ccs_ray_value;
    ccs_raycast_hit ccs_hit;
    int ccs_body_id;
    sicol_shape_t sicol_ray_value;
    sicol_world_raycast_hit_t sicol_hit;
    fx sicol_origin[3];
    fx sicol_direction[3];
    Blank3DCollisionHit from_ccs;
    Blank3DCollisionHit from_sicol;
    int has_ccs;
    int has_sicol;
    if (!collision || !collision->initialized || !origin || !direction || !out_hit)
        return 0;
    memset(out_hit, 0, sizeof(*out_hit));
    if (range_fx <= 0) return 0;

    ccs_ray_value = ccs_ray_make(b3d_ccs_vec(origin),
        ccs_vec3_normalize(b3d_ccs_vec(direction)), 0,
        b3d_q12_to_ccs(range_fx));
    ccs_body_id = -1;
    has_ccs = ccs_world_raycast(&ccs_ray_value, (ccs_u32)layer_mask,
                                &ccs_hit, &ccs_body_id);
    if (has_ccs)
        has_ccs = b3d_fill_ccs_hit(collision, &ccs_hit, ccs_body_id,
                                   range_fx, &from_ccs);

    b3d_sicol_vec(origin, sicol_origin);
    b3d_sicol_vec(direction, sicol_direction);
    (void)fx_normalize3(sicol_direction, sicol_direction);
    sicol_shape_make_ray(&sicol_ray_value, sicol_origin, sicol_direction,
                         b3d_q12_to_sicol(range_fx));
    has_sicol = sicol_world_raycast(&collision->sicol_world,
                                    &sicol_ray_value, (uint32_t)layer_mask,
                                    &sicol_hit);
    if (has_sicol)
        has_sicol = b3d_fill_sicol_ray_hit(collision, &sicol_hit,
                                           range_fx, &from_sicol);

    if (has_sicol && (!has_ccs || from_sicol.distance_fx <= from_ccs.distance_fx)) {
        *out_hit = from_sicol;
        return 1;
    }
    if (has_ccs) {
        *out_hit = from_ccs;
        return 1;
    }
    return 0;
}

int blank3d_collision_sweep_bullet_mask(Blank3DCollision *collision,
                                        const GWP89_Vec3 *start,
                                        const GWP89_Vec3 *delta,
                                        gwp89_fx radius_fx,
                                        unsigned int target_layers,
                                        Blank3DCollisionHit *out_hit)
{
    sicol_shape_t sphere;
    sicol_world_cast_hit_t cast_hit;
    fx start_fx[3];
    fx delta_fx[3];
    gwp89_fx path_length;
    int enemy_index;
    Blank3DCollisionHit ray_hit;
    GWP89_Vec3 direction;
    ccs_vec3 ccs_direction;
    if (!collision || !collision->initialized || !start || !delta || !out_hit)
        return 0;
    memset(out_hit, 0, sizeof(*out_hit));
    target_layers &= B3D_COLLISION_LAYER_ENEMY | B3D_COLLISION_LAYER_WORLD;
    if (target_layers == 0U) return 0;
    path_length = b3d_gwp_length(delta);
    if (path_length <= 0) return 0;

    b3d_sicol_vec(start, start_fx);
    b3d_sicol_vec(delta, delta_fx);
    if (radius_fx <= 0) radius_fx = GWP89_FIX_ONE / 20L;
    sicol_shape_make_sphere(&sphere, start_fx, b3d_q12_to_sicol(radius_fx));
    if (sicol_world_cast_shape_ex(&collision->sicol_world, &sphere, delta_fx,
            target_layers,
            B3D_COLLISION_LAYER_BULLET,
            target_layers,
            -1, &cast_hit)) {
        enemy_index = b3d_enemy_from_sicol(collision, cast_hit.id);
        if (enemy_index >= 0 || b3d_is_sicol_static(collision, cast_hit.id)) {
            out_hit->hit = 1;
            out_hit->enemy_index = enemy_index;
            out_hit->material_id = enemy_index >= 0
                ? B3D_COLLISION_MATERIAL_FLESH : B3D_COLLISION_MATERIAL_WORLD;
            out_hit->fraction_fx = b3d_sicol_to_q12(cast_hit.fraction);
            out_hit->distance_fx = (gwp89_fx)((path_length * out_hit->fraction_fx) /
                                               GWP89_FIX_ONE);
            out_hit->point = b3d_gwp_from_sicol(cast_hit.contact.point);
            out_hit->normal = b3d_gwp_from_sicol(cast_hit.contact.normal);
            return 1;
        }
    }

    /* CCS raycast is the deterministic fallback and broad confirmation path. */
    ccs_direction = ccs_vec3_normalize(b3d_ccs_vec(delta));
    direction = b3d_gwp_from_ccs(ccs_direction);
    if (blank3d_collision_raycast(collision, start, &direction, path_length,
            target_layers, &ray_hit)) {
        *out_hit = ray_hit;
        return 1;
    }
    return 0;
}

int blank3d_collision_sweep_bullet(Blank3DCollision *collision,
                                   const GWP89_Vec3 *start,
                                   const GWP89_Vec3 *delta,
                                   gwp89_fx radius_fx,
                                   Blank3DCollisionHit *out_hit)
{
    return blank3d_collision_sweep_bullet_mask(
        collision, start, delta, radius_fx,
        B3D_COLLISION_LAYER_ENEMY | B3D_COLLISION_LAYER_WORLD, out_hit);
}

int blank3d_collision_weapon_provider(void *context,
                                      GWP89_ProviderPacket *packet)
{
    Blank3DCollision *collision;
    Blank3DCollisionHit hit;
    collision = (Blank3DCollision *)context;
    if (!collision || !packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (packet->operation != GWP89_OP_RAYCAST) return GWP89_PROVIDER_PASS;
    if (blank3d_collision_raycast(collision, &packet->vec_a, &packet->vec_b,
            packet->fx_value,
            B3D_COLLISION_LAYER_ENEMY | B3D_COLLISION_LAYER_WORLD, &hit)) {
        packet->hit.hit = 1;
        packet->hit.actor_id = hit.enemy_index >= 0 ? 100 + hit.enemy_index : 0;
        packet->hit.material_id = hit.material_id;
        packet->hit.point = hit.point;
        packet->hit.normal = hit.normal;
        packet->hit.distance_fx = hit.distance_fx;
        return GWP89_PROVIDER_HANDLED;
    }
    packet->hit.hit = 0;
    return GWP89_PROVIDER_HANDLED;
}

const char *blank3d_collision_status(const Blank3DCollision *collision)
{
    return collision ? collision->status : "collision unavailable";
}
