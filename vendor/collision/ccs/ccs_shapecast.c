#include "ccs_shapecast.h"

#include "ccs_dispatch.h"

static ccs_fixed clamp01(ccs_fixed t)
{
    if (t < 0) return 0;
    if (t > CCS_FIXED_ONE) return CCS_FIXED_ONE;
    return t;
}

static int aabb_overlap(const ccs_aabb* a, const ccs_aabb* b)
{
    if (a->max.x < b->min.x || a->min.x > b->max.x) return 0;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return 0;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return 0;
    return 1;
}

/* Swept AABB(A) vs static AABB(B). A moves by delta over t in [0,1].
   Returns 1 if there is an overlap interval; outputs t_enter/t_exit.
*/
static int sweep_aabb_aabb(
    const ccs_aabb* a0,
    const ccs_aabb* b,
    ccs_vec3 delta,
    ccs_fixed* out_t_enter,
    ccs_fixed* out_t_exit
) {
    ccs_fixed t_enter;
    ccs_fixed t_exit;

    if (!a0 || !b || !out_t_enter || !out_t_exit)
        return 0;

    /* If already overlapping at t=0, interval starts at 0 */
    t_enter = 0;
    t_exit = CCS_FIXED_ONE;

    /* For each axis */
    {
        /* X */
        ccs_fixed v;
        v = delta.x;
        if (v == 0) {
            if (a0->max.x < b->min.x || a0->min.x > b->max.x)
                return 0;
        } else {
            ccs_fixed t0;
            ccs_fixed t1;
            t0 = ccs_fixed_div((b->min.x - a0->max.x), v);
            t1 = ccs_fixed_div((b->max.x - a0->min.x), v);
            if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
            if (t0 > t_enter) t_enter = t0;
            if (t1 < t_exit) t_exit = t1;
            if (t_enter > t_exit) return 0;
        }

        /* Y */
        v = delta.y;
        if (v == 0) {
            if (a0->max.y < b->min.y || a0->min.y > b->max.y)
                return 0;
        } else {
            ccs_fixed t0;
            ccs_fixed t1;
            t0 = ccs_fixed_div((b->min.y - a0->max.y), v);
            t1 = ccs_fixed_div((b->max.y - a0->min.y), v);
            if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
            if (t0 > t_enter) t_enter = t0;
            if (t1 < t_exit) t_exit = t1;
            if (t_enter > t_exit) return 0;
        }

        /* Z */
        v = delta.z;
        if (v == 0) {
            if (a0->max.z < b->min.z || a0->min.z > b->max.z)
                return 0;
        } else {
            ccs_fixed t0;
            ccs_fixed t1;
            t0 = ccs_fixed_div((b->min.z - a0->max.z), v);
            t1 = ccs_fixed_div((b->max.z - a0->min.z), v);
            if (t0 > t1) { ccs_fixed tmp = t0; t0 = t1; t1 = tmp; }
            if (t0 > t_enter) t_enter = t0;
            if (t1 < t_exit) t_exit = t1;
            if (t_enter > t_exit) return 0;
        }
    }

    /* clamp to [0,1] */
    t_enter = clamp01(t_enter);
    t_exit = clamp01(t_exit);

    if (t_enter > t_exit)
        return 0;

    *out_t_enter = t_enter;
    *out_t_exit = t_exit;
    return 1;
}

int ccs_shapecast_shape(
    void* moving_shape,
    ccs_vec3 start_center,
    ccs_vec3 end_center,
    const void* target_shape,
    ccs_shapecast_hit* out
) {
    ccs_vec3 original_center;
    ccs_vec3 delta;
    ccs_fixed delta_len_sq;

    ccs_aabb a0;
    ccs_aabb a1;
    ccs_aabb b;

    ccs_fixed t_enter;
    ccs_fixed t_exit;

    int i;
    int found;

    if (!moving_shape || !target_shape || !out)
        return 0;

    out->hit = 0;

    original_center = ccs_shape_get_center(moving_shape);

    /* initial overlap */
    ccs_shape_set_center(moving_shape, start_center);
    if (ccs_dispatch_test(moving_shape, target_shape)) {
        ccs_contact c;
        out->hit = 1;
        out->fraction = 0;
        out->point = start_center;
        if (ccs_dispatch_collide(moving_shape, target_shape, &c)) {
            out->normal = c.normal;
            out->penetration = c.penetration;
        } else {
            out->normal = ccs_vec3_axis_y();
            out->penetration = 0;
        }
        ccs_shape_set_center(moving_shape, original_center);
        return 1;
    }

    delta = ccs_vec3_sub(end_center, start_center);
    delta_len_sq = ccs_vec3_len_sq(delta);
    if (delta_len_sq == 0) {
        ccs_shape_set_center(moving_shape, original_center);
        return 0;
    }

    /* compute AABBs */
    ccs_shape_set_center(moving_shape, start_center);
    a0 = ccs_shape_compute_aabb(moving_shape);

    ccs_shape_set_center(moving_shape, end_center);
    a1 = ccs_shape_compute_aabb(moving_shape);

    b = ccs_shape_compute_aabb(target_shape);

    /* quick reject using union AABB */
    {
        ccs_aabb uni;
        uni.min = ccs_vec3_min(a0.min, a1.min);
        uni.max = ccs_vec3_max(a0.max, a1.max);
        if (!aabb_overlap(&uni, &b)) {
            ccs_shape_set_center(moving_shape, original_center);
            return 0;
        }
    }

    if (!sweep_aabb_aabb(&a0, &b, delta, &t_enter, &t_exit)) {
        ccs_shape_set_center(moving_shape, original_center);
        return 0;
    }

    found = 0;

    /* sampling within [t_enter, t_exit] */
    {
        ccs_fixed interval;
        ccs_fixed prev_t;

        interval = t_exit - t_enter;
        prev_t = t_enter;

        /* ensure prev_t isn't overlapping (it shouldn't, but be safe) */
        {
            ccs_vec3 c;
            c = ccs_vec3_add(start_center, ccs_vec3_scale(delta, prev_t));
            ccs_shape_set_center(moving_shape, c);
            if (ccs_dispatch_test(moving_shape, target_shape)) {
                /* pathological: treat as hit at prev_t */
                found = 1;
                out->fraction = prev_t;
            }
        }

        if (!found) {
            for (i = 1; i <= CCS_SHAPECAST_SAMPLES; ++i) {
                ccs_fixed fk;
                ccs_fixed t;
                ccs_vec3 c;

                fk = ccs_fixed_div(ccs_fixed_from_int((ccs_i32)i), ccs_fixed_from_int((ccs_i32)CCS_SHAPECAST_SAMPLES));
                t = t_enter + ccs_fixed_mul(interval, fk);

                c = ccs_vec3_add(start_center, ccs_vec3_scale(delta, t));
                ccs_shape_set_center(moving_shape, c);

                if (ccs_dispatch_test(moving_shape, target_shape)) {
                    /* bracket found: [prev_t, t] */
                    ccs_fixed lo;
                    ccs_fixed hi;
                    int it;

                    lo = prev_t;
                    hi = t;

                    for (it = 0; it < CCS_SHAPECAST_ITERATIONS; ++it) {
                        ccs_fixed mid;
                        ccs_vec3 cm;
                        mid = (lo + hi) >> 1;
                        cm = ccs_vec3_add(start_center, ccs_vec3_scale(delta, mid));
                        ccs_shape_set_center(moving_shape, cm);
                        if (ccs_dispatch_test(moving_shape, target_shape)) {
                            hi = mid;
                        } else {
                            lo = mid;
                        }
                    }

                    out->fraction = hi;
                    found = 1;
                    break;
                }

                prev_t = t;
            }
        }
    }

    if (!found) {
        ccs_shape_set_center(moving_shape, original_center);
        return 0;
    }

    /* finalize */
    {
        ccs_vec3 c;
        ccs_contact contact;

        c = ccs_vec3_add(start_center, ccs_vec3_scale(delta, out->fraction));
        ccs_shape_set_center(moving_shape, c);

        out->hit = 1;
        out->point = c;

        if (ccs_dispatch_collide(moving_shape, target_shape, &contact)) {
            out->normal = contact.normal;
            out->penetration = contact.penetration;
        } else {
            out->normal = ccs_vec3_axis_y();
            out->penetration = 0;
        }
    }

    ccs_shape_set_center(moving_shape, original_center);

    return 1;
}
