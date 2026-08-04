#include "blank3d_casing_physics.h"

#include <string.h>

static vp_u32 b3d_casing_physics_id(int slot)
{
    if (slot < 0 || slot >= B3D_CASING_PHYSICS_CAPACITY) return 0UL;
    return (vp_u32)(B3D_CASING_PHYSICS_ID_BASE + (unsigned long)slot);
}

static g3d_fix b3d_ratio(long numerator, long denominator)
{
    if (denominator == 0L) return 0;
    return (g3d_fix)((numerator * G3D_FIX_ONE) / denominator);
}

static void b3d_casing_profile(int mesh_id,
                               g3d_fix *half_x,
                               g3d_fix *half_y,
                               g3d_fix *half_z,
                               g3d_fix *mass,
                               g3d_fix *restitution)
{
    g3d_fix hx;
    g3d_fix hy;
    g3d_fix hz;
    g3d_fix m;
    g3d_fix bounce;
    hx = b3d_ratio(1L, 20L);
    hy = b3d_ratio(1L, 20L);
    hz = b3d_ratio(7L, 50L);
    m = b3d_ratio(1L, 20L);
    bounce = b3d_ratio(11L, 20L);
    switch (mesh_id) {
    case 2: /* machine-gun / rifle brass */
        hx = b3d_ratio(9L, 200L);
        hy = b3d_ratio(9L, 200L);
        hz = b3d_ratio(4L, 25L);
        m = b3d_ratio(3L, 50L);
        bounce = b3d_ratio(1L, 2L);
        break;
    case 3: /* shotgun plastic/brass shell */
        hx = b3d_ratio(7L, 100L);
        hy = b3d_ratio(7L, 100L);
        hz = b3d_ratio(13L, 50L);
        m = b3d_ratio(1L, 10L);
        bounce = b3d_ratio(9L, 20L);
        break;
    case 4: /* magnum brass */
        hx = b3d_ratio(3L, 50L);
        hy = b3d_ratio(3L, 50L);
        hz = b3d_ratio(9L, 50L);
        m = b3d_ratio(7L, 100L);
        bounce = b3d_ratio(1L, 2L);
        break;
    case 5: /* long rifle / sniper brass */
        hx = b3d_ratio(11L, 200L);
        hy = b3d_ratio(11L, 200L);
        hz = b3d_ratio(11L, 50L);
        m = b3d_ratio(2L, 25L);
        bounce = b3d_ratio(9L, 20L);
        break;
    case 1:
    default:
        break;
    }
    if (half_x) *half_x = hx;
    if (half_y) *half_y = hy;
    if (half_z) *half_z = hz;
    if (mass) *mass = m;
    if (restitution) *restitution = bounce;
}

static Vec3 b3d_scaled_sum(const Vec3 *right,
                           g3d_fix right_scale,
                           const Vec3 *up,
                           g3d_fix up_scale,
                           const Vec3 *forward,
                           g3d_fix forward_scale)
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
    Vec3 sum;
    Vec3 result;
    a = gamlib_vec3(0, 0, 0);
    b = gamlib_vec3(0, 0, 0);
    c = gamlib_vec3(0, 0, 0);
    if (right) gamlib_vec3_scale(&a, right, right_scale);
    if (up) gamlib_vec3_scale(&b, up, up_scale);
    if (forward) gamlib_vec3_scale(&c, forward, forward_scale);
    gamlib_vec3_add(&sum, &a, &b);
    gamlib_vec3_add(&result, &sum, &c);
    return result;
}

void blank3d_casing_physics_init(Blank3DCasingPhysics *casings,
                                 Blank3DVPhysics *physics)
{
    if (!casings) return;
    memset(casings, 0, sizeof(*casings));
    casings->physics = physics;
}

int blank3d_casing_physics_spawn(Blank3DCasingPhysics *casings,
                                 int slot,
                                 int weapon_id,
                                 int casing_mesh_id,
                                 const Vec3 *origin,
                                 const Vec3 *right,
                                 const Vec3 *up,
                                 const Vec3 *forward)
{
    vp_u32 id;
    g3d_fix half_x;
    g3d_fix half_y;
    g3d_fix half_z;
    g3d_fix mass;
    g3d_fix restitution;
    g3d_fix right_speed;
    g3d_fix up_speed;
    g3d_fix forward_speed;
    Vec3 velocity;
    Vec3 angular;
    if (!casings || !casings->physics || !origin) return 0;
    id = b3d_casing_physics_id(slot);
    if (id == 0UL) return 0;
    (void)blank3d_vphysics_destroy_object(casings->physics, id);
    b3d_casing_profile(casing_mesh_id, &half_x, &half_y, &half_z,
                       &mass, &restitution);
    if (!blank3d_vphysics_create_dynamic_box_q12(
            casings->physics, id,
            origin->x, origin->y, origin->z,
            half_x, half_y, half_z, mass)) {
        casings->rejected += 1UL;
        return 0;
    }

    /* Casings collide with the world but not with each other. That avoids an
     * O(n^2) brass-cloud while preserving floor, wall and prop response. */
    (void)blank3d_vphysics_configure_object_q12(
        casings->physics, id,
        b3d_ratio(13L, 20L), restitution,
        b3d_ratio(7L, 20L), b3d_ratio(16L, 5L),
        0, 0);
    /* Shell ejection speeds are low enough for discrete contacts. Disabling
     * per-body CCD prevents repeated TOI substeps from consuming restitution
     * in the same frame and flattening the visible rebound. */
    (void)blank3d_vphysics_set_ccd_enabled(casings->physics, id, 0);

    right_speed = weapon_id == 8 ? G3D_FIX_FROM_INT(5) :
                  (weapon_id == 2 ? G3D_FIX_FROM_INT(4) :
                   G3D_FIX_FROM_INT(3));
    up_speed = casing_mesh_id == 3 ? G3D_FIX_FROM_INT(3) :
               G3D_FIX_FROM_INT(2);
    forward_speed = b3d_ratio(7L, 20L);
    velocity = b3d_scaled_sum(right, right_speed,
                              up, up_speed,
                              forward, forward_speed);

    /* VPhysics angular velocity is radians/second. Deterministic slot-based
     * variation prevents every shell from sharing exactly the same tumble. */
    angular.x = G3D_FIX_FROM_INT(14 + (slot % 5) * 2);
    angular.y = G3D_FIX_FROM_INT(22 + (slot % 7) * 2);
    angular.z = G3D_FIX_FROM_INT(17 + (slot % 3) * 3);
    if (!blank3d_vphysics_set_velocity_q12(
            casings->physics, id,
            velocity.x, velocity.y, velocity.z,
            angular.x, angular.y, angular.z)) {
        blank3d_vphysics_destroy_object(casings->physics, id);
        casings->rejected += 1UL;
        return 0;
    }
    casings->spawned += 1UL;
    return 1;
}

void blank3d_casing_physics_release(Blank3DCasingPhysics *casings,
                                    int slot)
{
    vp_u32 id;
    if (!casings || !casings->physics) return;
    id = b3d_casing_physics_id(slot);
    if (id != 0UL)
        (void)blank3d_vphysics_destroy_object(casings->physics, id);
}

int blank3d_casing_physics_get_draw_matrix_q16(
    const Blank3DCasingPhysics *casings,
    int slot,
    g3d_fix visual_scale,
    signed int out_matrix[4][4])
{
    vp_u32 id;
    if (!casings || !casings->physics || !out_matrix) return 0;
    id = b3d_casing_physics_id(slot);
    if (id == 0UL) return 0;
    return blank3d_vphysics_get_draw_matrix_scaled_q12(
        casings->physics, id,
        visual_scale, visual_scale, visual_scale,
        out_matrix);
}

int blank3d_casing_physics_is_sleeping(
    const Blank3DCasingPhysics *casings,
    int slot)
{
    vp_u32 id;
    if (!casings || !casings->physics) return 0;
    id = b3d_casing_physics_id(slot);
    if (id == 0UL) return 0;
    return blank3d_vphysics_body_is_asleep(casings->physics, id);
}

int blank3d_casing_physics_get_velocity(
    const Blank3DCasingPhysics *casings,
    int slot,
    Vec3 *linear,
    Vec3 *angular)
{
    vp_u32 id;
    long vx;
    long vy;
    long vz;
    long wx;
    long wy;
    long wz;
    if (!casings || !casings->physics) return 0;
    id = b3d_casing_physics_id(slot);
    if (id == 0UL) return 0;
    if (!blank3d_vphysics_get_velocity_q12(casings->physics, id,
            &vx, &vy, &vz, &wx, &wy, &wz)) return 0;
    if (linear) *linear = gamlib_vec3(vx, vy, vz);
    if (angular) *angular = gamlib_vec3(wx, wy, wz);
    return 1;
}

unsigned long blank3d_casing_physics_spawned(
    const Blank3DCasingPhysics *casings)
{
    return casings ? casings->spawned : 0UL;
}

unsigned long blank3d_casing_physics_rejected(
    const Blank3DCasingPhysics *casings)
{
    return casings ? casings->rejected : 0UL;
}
