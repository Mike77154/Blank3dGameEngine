#include "ccs_geom.h"

void ccs_obb_set_identity(ccs_obb* o)
{
    if (!o) return;
    o->axis[0] = ccs_vec3_axis_x();
    o->axis[1] = ccs_vec3_axis_y();
    o->axis[2] = ccs_vec3_axis_z();
}

ccs_vec3 ccs_obb_to_local_point(const ccs_obb* o, ccs_vec3 p)
{
    ccs_vec3 d;
    ccs_vec3 out;

    if (!o)
        return ccs_vec3_zero();

    d = ccs_vec3_sub(p, o->center);

    out.x = ccs_vec3_dot(d, o->axis[0]);
    out.y = ccs_vec3_dot(d, o->axis[1]);
    out.z = ccs_vec3_dot(d, o->axis[2]);

    return out;
}

ccs_vec3 ccs_obb_to_local_dir(const ccs_obb* o, ccs_vec3 v)
{
    ccs_vec3 out;

    if (!o)
        return ccs_vec3_zero();

    out.x = ccs_vec3_dot(v, o->axis[0]);
    out.y = ccs_vec3_dot(v, o->axis[1]);
    out.z = ccs_vec3_dot(v, o->axis[2]);

    return out;
}

ccs_vec3 ccs_obb_to_world_point(const ccs_obb* o, ccs_vec3 p_local)
{
    ccs_vec3 out;

    if (!o)
        return ccs_vec3_zero();

    out = o->center;
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[0], p_local.x));
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[1], p_local.y));
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[2], p_local.z));

    return out;
}

ccs_vec3 ccs_obb_to_world_dir(const ccs_obb* o, ccs_vec3 v_local)
{
    ccs_vec3 out;

    if (!o)
        return ccs_vec3_zero();

    out = ccs_vec3_zero();
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[0], v_local.x));
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[1], v_local.y));
    out = ccs_vec3_add(out, ccs_vec3_scale(o->axis[2], v_local.z));

    return out;
}

ccs_vec3 ccs_obb_support(const ccs_obb* o, ccs_vec3 dir_world)
{
    ccs_vec3 result;
    ccs_fixed sx;
    ccs_fixed sy;
    ccs_fixed sz;

    if (!o)
        return ccs_vec3_zero();

    /* choose sign based on projection on each axis */
    sx = ccs_vec3_dot(dir_world, o->axis[0]);
    sy = ccs_vec3_dot(dir_world, o->axis[1]);
    sz = ccs_vec3_dot(dir_world, o->axis[2]);

    result = o->center;

    result = ccs_vec3_add(result,
        ccs_vec3_scale(o->axis[0], (sx >= 0) ? o->half.x : -o->half.x));

    result = ccs_vec3_add(result,
        ccs_vec3_scale(o->axis[1], (sy >= 0) ? o->half.y : -o->half.y));

    result = ccs_vec3_add(result,
        ccs_vec3_scale(o->axis[2], (sz >= 0) ? o->half.z : -o->half.z));

    return result;
}

void ccs_capsule_endpoints(const ccs_capsule* c, ccs_vec3* out_a, ccs_vec3* out_b)
{
    ccs_vec3 off;

    if (!c) {
        if (out_a) *out_a = ccs_vec3_zero();
        if (out_b) *out_b = ccs_vec3_zero();
        return;
    }

    off = ccs_vec3_scale(c->axis, c->half_height);

    if (out_a)
        *out_a = ccs_vec3_sub(c->center, off);
    if (out_b)
        *out_b = ccs_vec3_add(c->center, off);
}
