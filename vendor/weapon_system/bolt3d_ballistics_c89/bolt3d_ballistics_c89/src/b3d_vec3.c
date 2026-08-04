#include "bolt3d/b3d_vec3.h"

B3D_Vec3 b3d_vec3(B3D_Fixed x, B3D_Fixed y, B3D_Fixed z)
{
    B3D_Vec3 v;

    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

B3D_Vec3 b3d_vec3_zero(void)
{
    return b3d_vec3(0, 0, 0);
}

B3D_Vec3 b3d_vec3_add(B3D_Vec3 a, B3D_Vec3 b)
{
    B3D_Vec3 r;

    r.x = b3d_fixed_add_sat(a.x, b.x);
    r.y = b3d_fixed_add_sat(a.y, b.y);
    r.z = b3d_fixed_add_sat(a.z, b.z);
    return r;
}

B3D_Vec3 b3d_vec3_sub(B3D_Vec3 a, B3D_Vec3 b)
{
    B3D_Vec3 r;

    r.x = b3d_fixed_sub_sat(a.x, b.x);
    r.y = b3d_fixed_sub_sat(a.y, b.y);
    r.z = b3d_fixed_sub_sat(a.z, b.z);
    return r;
}

B3D_Vec3 b3d_vec3_scale(B3D_Vec3 v, B3D_Fixed scale)
{
    B3D_Vec3 r;

    r.x = b3d_fixed_mul(v.x, scale);
    r.y = b3d_fixed_mul(v.y, scale);
    r.z = b3d_fixed_mul(v.z, scale);
    return r;
}

B3D_Fixed b3d_vec3_dot(B3D_Vec3 a, B3D_Vec3 b)
{
    B3D_Fixed x;
    B3D_Fixed y;
    B3D_Fixed z;
    B3D_Fixed xy;

    x = b3d_fixed_mul(a.x, b.x);
    y = b3d_fixed_mul(a.y, b.y);
    z = b3d_fixed_mul(a.z, b.z);
    xy = b3d_fixed_add_sat(x, y);
    return b3d_fixed_add_sat(xy, z);
}

B3D_Vec3 b3d_vec3_cross(B3D_Vec3 a, B3D_Vec3 b)
{
    B3D_Vec3 r;

    r.x = b3d_fixed_sub_sat(b3d_fixed_mul(a.y, b.z), b3d_fixed_mul(a.z, b.y));
    r.y = b3d_fixed_sub_sat(b3d_fixed_mul(a.z, b.x), b3d_fixed_mul(a.x, b.z));
    r.z = b3d_fixed_sub_sat(b3d_fixed_mul(a.x, b.y), b3d_fixed_mul(a.y, b.x));
    return r;
}

B3D_Vec3 b3d_vec3_mul_components(B3D_Vec3 a, B3D_Vec3 b)
{
    return b3d_vec3(b3d_fixed_mul(a.x, b.x), b3d_fixed_mul(a.y, b.y), b3d_fixed_mul(a.z, b.z));
}

B3D_Vec3 b3d_vec3_div_components(B3D_Vec3 a, B3D_Vec3 b)
{
    B3D_Vec3 r;

    r.x = b.x != 0 ? b3d_fixed_div(a.x, b.x) : 0;
    r.y = b.y != 0 ? b3d_fixed_div(a.y, b.y) : 0;
    r.z = b.z != 0 ? b3d_fixed_div(a.z, b.z) : 0;
    return r;
}

B3D_Fixed b3d_vec3_length_sq(B3D_Vec3 v)
{
    return b3d_vec3_dot(v, v);
}

B3D_Fixed b3d_vec3_length(B3D_Vec3 v)
{
    return b3d_fixed_sqrt(b3d_vec3_length_sq(v));
}

B3D_Vec3 b3d_vec3_normalize(B3D_Vec3 v)
{
    B3D_Fixed len;
    B3D_Vec3 r;

    len = b3d_vec3_length(v);
    if (len <= B3D_FIXED_EPSILON) {
        return b3d_vec3_zero();
    }

    r.x = b3d_fixed_div(v.x, len);
    r.y = b3d_fixed_div(v.y, len);
    r.z = b3d_fixed_div(v.z, len);
    return r;
}

B3D_Vec3 b3d_vec3_lerp(B3D_Vec3 a, B3D_Vec3 b, B3D_Fixed t)
{
    B3D_Vec3 r;

    r.x = b3d_fixed_lerp(a.x, b.x, t);
    r.y = b3d_fixed_lerp(a.y, b.y, t);
    r.z = b3d_fixed_lerp(a.z, b.z, t);
    return r;
}

B3D_Vec3 b3d_vec3_reflect(B3D_Vec3 v, B3D_Vec3 normal)
{
    B3D_Fixed dot_value;
    B3D_Vec3 scaled;

    dot_value = b3d_vec3_dot(v, normal);
    scaled = b3d_vec3_scale(normal, b3d_fixed_mul_int(dot_value, 2));
    return b3d_vec3_sub(v, scaled);
}

B3D_Vec3 b3d_vec3_project(B3D_Vec3 v, B3D_Vec3 normal)
{
    B3D_Vec3 n;

    n = b3d_vec3_normalize(normal);
    return b3d_vec3_scale(n, b3d_vec3_dot(v, n));
}

B3D_Vec3 b3d_vec3_reject(B3D_Vec3 v, B3D_Vec3 normal)
{
    return b3d_vec3_sub(v, b3d_vec3_project(v, normal));
}

B3D_Fixed b3d_vec3_distance_sq(B3D_Vec3 a, B3D_Vec3 b)
{
    return b3d_vec3_length_sq(b3d_vec3_sub(a, b));
}

B3D_Fixed b3d_vec3_distance(B3D_Vec3 a, B3D_Vec3 b)
{
    return b3d_fixed_sqrt(b3d_vec3_distance_sq(a, b));
}
