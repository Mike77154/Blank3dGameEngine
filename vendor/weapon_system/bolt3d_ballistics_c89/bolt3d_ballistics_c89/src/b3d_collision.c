#include "bolt3d/b3d_collision.h"

void b3d_hit_clear(B3D_Hit *hit)
{
    if (hit == 0) {
        return;
    }

    hit->hit = B3D_FALSE;
    hit->hit_kind = B3D_HIT_NONE;
    hit->target_id = B3D_ID_NONE;
    hit->target_slot = B3D_ID_NONE;
    hit->target_layer = 0;
    hit->target_user_type = 0;
    hit->t = 0;
    hit->point = b3d_vec3_zero();
    hit->normal = b3d_vec3_zero();
    hit->user_ptr = 0;
}

void b3d_collider_clear(B3D_Collider *collider)
{
    if (collider == 0) {
        return;
    }

    collider->active = B3D_FALSE;
    collider->id = B3D_ID_NONE;
    collider->layer_mask = 0;
    collider->center = b3d_vec3_zero();
    collider->radius = 0;
    collider->user_type = 0;
    collider->user_ptr = 0;
}

int b3d_sphere_overlap(B3D_Vec3 a, B3D_Fixed radius_a, B3D_Vec3 b, B3D_Fixed radius_b)
{
    B3D_Fixed radius;
    B3D_Fixed radius_sq;
    B3D_Fixed dist_sq;

    radius = b3d_fixed_add_sat(radius_a, radius_b);
    radius_sq = b3d_fixed_mul(radius, radius);
    dist_sq = b3d_vec3_distance_sq(a, b);

    return dist_sq <= radius_sq;
}

int b3d_segment_sphere(B3D_Vec3 from, B3D_Vec3 to, B3D_Vec3 center, B3D_Fixed radius, B3D_Hit *out_hit)
{
    B3D_Vec3 d;
    B3D_Vec3 m;
    B3D_Vec3 point;
    B3D_Fixed a;
    B3D_Fixed b;
    B3D_Fixed c;
    B3D_Fixed radius_sq;
    B3D_Fixed disc;
    B3D_Fixed root;
    B3D_Fixed t;
    B3D_Fixed denom;
    B3D_Fixed four_ac;
    B3D_Hit local_hit;

    b3d_hit_clear(&local_hit);

    d = b3d_vec3_sub(to, from);
    m = b3d_vec3_sub(from, center);
    a = b3d_vec3_dot(d, d);
    radius_sq = b3d_fixed_mul(radius, radius);
    c = b3d_fixed_sub_sat(b3d_vec3_dot(m, m), radius_sq);

    if (a <= B3D_FIXED_EPSILON) {
        if (c <= 0) {
            local_hit.hit = B3D_TRUE;
            local_hit.hit_kind = B3D_HIT_ENTITY;
            local_hit.t = 0;
            local_hit.point = from;
            local_hit.normal = b3d_vec3_normalize(b3d_vec3_sub(from, center));
            if (out_hit != 0) {
                *out_hit = local_hit;
            }
            return B3D_TRUE;
        }
        return B3D_FALSE;
    }

    if (c <= 0) {
        local_hit.hit = B3D_TRUE;
        local_hit.hit_kind = B3D_HIT_ENTITY;
        local_hit.t = 0;
        local_hit.point = from;
        local_hit.normal = b3d_vec3_normalize(b3d_vec3_sub(from, center));
        if (out_hit != 0) {
            *out_hit = local_hit;
        }
        return B3D_TRUE;
    }

    b = b3d_fixed_mul_int(b3d_vec3_dot(m, d), 2);
    if (b > 0) {
        return B3D_FALSE;
    }

    four_ac = b3d_fixed_mul_int(b3d_fixed_mul(a, c), 4);
    disc = b3d_fixed_sub_sat(b3d_fixed_mul(b, b), four_ac);
    if (disc < 0) {
        return B3D_FALSE;
    }

    root = b3d_fixed_sqrt(disc);
    denom = b3d_fixed_mul_int(a, 2);
    if (denom <= B3D_FIXED_EPSILON) {
        return B3D_FALSE;
    }

    t = b3d_fixed_div(b3d_fixed_sub_sat(b3d_fixed_neg(b), root), denom);
    if (t < 0 || t > B3D_FIXED_ONE) {
        return B3D_FALSE;
    }

    point = b3d_vec3_lerp(from, to, t);
    local_hit.hit = B3D_TRUE;
    local_hit.hit_kind = B3D_HIT_ENTITY;
    local_hit.t = t;
    local_hit.point = point;
    local_hit.normal = b3d_vec3_normalize(b3d_vec3_sub(point, center));

    if (out_hit != 0) {
        *out_hit = local_hit;
    }
    return B3D_TRUE;
}
