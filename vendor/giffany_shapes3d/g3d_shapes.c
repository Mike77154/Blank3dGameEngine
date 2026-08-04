#include "g3d_shapes.h"

static unsigned long g3d_uabs_long(g3d_fx value)
{
    unsigned long u;
    u = (unsigned long)value;
    if (value < 0) {
        u = 0UL - u;
    }
    return u;
}

static g3d_fx g3d_from_magnitude(unsigned long magnitude, int negative)
{
    if (negative) {
        if (magnitude >= 0x80000000UL) {
            return G3D_FX_MIN;
        }
        return -(g3d_fx)magnitude;
    }
    if (magnitude > 0x7FFFFFFFUL) {
        return G3D_FX_MAX;
    }
    return (g3d_fx)magnitude;
}

static unsigned long g3d_uadd_limit(unsigned long a, unsigned long b,
                                    unsigned long limit, int *overflow)
{
    if (a > limit || b > limit - a) {
        *overflow = 1;
        return limit;
    }
    return a + b;
}

static g3d_fx g3d_fx_neg(g3d_fx value)
{
    if (value == G3D_FX_MIN) {
        return G3D_FX_MAX;
    }
    return -value;
}

static g3d_fx g3d_fx_min2(g3d_fx a, g3d_fx b)
{
    return a < b ? a : b;
}

static g3d_fx g3d_fx_max2(g3d_fx a, g3d_fx b)
{
    return a > b ? a : b;
}

g3d_fx g3d_fx_from_int(long value)
{
    if (value > 32767L) {
        return G3D_FX_MAX;
    }
    if (value < -32768L) {
        return G3D_FX_MIN;
    }
    return value * G3D_FX_ONE;
}

g3d_fx g3d_fx_from_ratio(long numerator, long denominator)
{
    return g3d_fx_div(g3d_fx_from_int(numerator), g3d_fx_from_int(denominator));
}

long g3d_fx_to_int(g3d_fx value)
{
    if (value >= 0) {
        return value / G3D_FX_ONE;
    }
    return -((g3d_fx_neg(value)) / G3D_FX_ONE);
}

g3d_fx g3d_fx_floor(g3d_fx value)
{
    g3d_fx remainder;
    if (value >= 0) {
        return (value / G3D_FX_ONE) * G3D_FX_ONE;
    }
    remainder = g3d_fx_neg(value) & 0xFFFFL;
    if (remainder == 0) {
        return value;
    }
    return -(((g3d_fx_neg(value) / G3D_FX_ONE) + 1L) * G3D_FX_ONE);
}

g3d_fx g3d_fx_abs(g3d_fx value)
{
    return value < 0 ? g3d_fx_neg(value) : value;
}

g3d_fx g3d_fx_add(g3d_fx a, g3d_fx b)
{
    if (b > 0 && a > G3D_FX_MAX - b) {
        return G3D_FX_MAX;
    }
    if (b < 0 && a < G3D_FX_MIN - b) {
        return G3D_FX_MIN;
    }
    return a + b;
}

g3d_fx g3d_fx_sub(g3d_fx a, g3d_fx b)
{
    if (b == G3D_FX_MIN) {
        return a >= 0 ? G3D_FX_MAX : g3d_fx_add(a, G3D_FX_MAX);
    }
    return g3d_fx_add(a, -b);
}

g3d_fx g3d_fx_mul(g3d_fx a, g3d_fx b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long ah;
    unsigned long al;
    unsigned long bh;
    unsigned long bl;
    unsigned long hh;
    unsigned long result;
    unsigned long limit;
    int negative;
    int overflow;

    if (a == 0 || b == 0) {
        return 0;
    }

    negative = ((a < 0) != (b < 0));
    ua = g3d_uabs_long(a);
    ub = g3d_uabs_long(b);
    ah = ua >> 16;
    al = ua & 0xFFFFUL;
    bh = ub >> 16;
    bl = ub & 0xFFFFUL;
    limit = negative ? 0x80000000UL : 0x7FFFFFFFUL;
    overflow = 0;

    hh = ah * bh;
    if (hh > (limit >> 16)) {
        return negative ? G3D_FX_MIN : G3D_FX_MAX;
    }
    result = hh << 16;
    result = g3d_uadd_limit(result, ah * bl, limit, &overflow);
    result = g3d_uadd_limit(result, al * bh, limit, &overflow);
    result = g3d_uadd_limit(result, (al * bl) >> 16, limit, &overflow);
    if (overflow) {
        return negative ? G3D_FX_MIN : G3D_FX_MAX;
    }
    return g3d_from_magnitude(result, negative);
}

g3d_fx g3d_fx_div(g3d_fx a, g3d_fx b)
{
    unsigned long ua;
    unsigned long ub;
    unsigned long integer_part;
    unsigned long remainder;
    unsigned long fraction;
    unsigned long result;
    unsigned long limit;
    int negative;
    int i;

    if (b == 0) {
        return a < 0 ? G3D_FX_MIN : G3D_FX_MAX;
    }
    if (a == 0) {
        return 0;
    }

    negative = ((a < 0) != (b < 0));
    ua = g3d_uabs_long(a);
    ub = g3d_uabs_long(b);
    integer_part = ua / ub;
    remainder = ua % ub;
    limit = negative ? 0x80000000UL : 0x7FFFFFFFUL;

    if (integer_part > (limit >> 16)) {
        return negative ? G3D_FX_MIN : G3D_FX_MAX;
    }

    fraction = 0UL;
    for (i = 0; i < 16; ++i) {
        fraction <<= 1;
        if (remainder >= ub - remainder) {
            remainder = remainder - (ub - remainder);
            fraction |= 1UL;
        } else {
            remainder += remainder;
        }
    }

    result = (integer_part << 16) | fraction;
    if (result > limit) {
        return negative ? G3D_FX_MIN : G3D_FX_MAX;
    }
    return g3d_from_magnitude(result, negative);
}

g3d_fx g3d_fx_sqrt(g3d_fx value)
{
    g3d_fx x;
    g3d_fx next;
    int i;
    if (value <= 0) {
        return 0;
    }
    x = value > G3D_FX_ONE ? value : G3D_FX_ONE;
    for (i = 0; i < 20; ++i) {
        next = g3d_fx_add(x, g3d_fx_div(value, x)) / 2L;
        if (g3d_fx_abs(g3d_fx_sub(next, x)) <= 1L) {
            x = next;
            break;
        }
        x = next;
    }
    return x;
}

g3d_fx g3d_fx_clamp(g3d_fx value, g3d_fx low, g3d_fx high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

g3d_fx g3d_fx_lerp(g3d_fx a, g3d_fx b, g3d_fx t)
{
    return g3d_fx_add(a, g3d_fx_mul(g3d_fx_sub(b, a), t));
}

void g3d_sincos(g3d_angle angle, g3d_fx *out_sine, g3d_fx *out_cosine)
{
    static const g3d_fx atan_table[15] = {
        8192L, 4836L, 2555L, 1297L, 651L,
        326L, 163L, 81L, 41L, 20L,
        10L, 5L, 3L, 1L, 1L
    };
    unsigned long a;
    unsigned short quadrant;
    g3d_fx z;
    g3d_fx x;
    g3d_fx y;
    g3d_fx nx;
    int i;
    int sin_sign;
    int cos_sign;

    if (angle == G3D_TURN_0) {
        if (out_sine != 0) *out_sine = 0;
        if (out_cosine != 0) *out_cosine = G3D_FX_ONE;
        return;
    }
    if (angle == G3D_TURN_90) {
        if (out_sine != 0) *out_sine = G3D_FX_ONE;
        if (out_cosine != 0) *out_cosine = 0;
        return;
    }
    if (angle == G3D_TURN_180) {
        if (out_sine != 0) *out_sine = 0;
        if (out_cosine != 0) *out_cosine = -G3D_FX_ONE;
        return;
    }
    if (angle == G3D_TURN_270) {
        if (out_sine != 0) *out_sine = -G3D_FX_ONE;
        if (out_cosine != 0) *out_cosine = 0;
        return;
    }

    a = (unsigned long)angle;
    quadrant = (unsigned short)((a >> 14) & 3UL);
    sin_sign = 1;
    cos_sign = 1;

    if (quadrant == 0U) {
        z = (g3d_fx)a;
    } else if (quadrant == 1U) {
        z = (g3d_fx)(32768UL - a);
        cos_sign = -1;
    } else if (quadrant == 2U) {
        z = (g3d_fx)(a - 32768UL);
        sin_sign = -1;
        cos_sign = -1;
    } else {
        z = (g3d_fx)(65536UL - a);
        sin_sign = -1;
    }

    x = 39797L;
    y = 0L;
    for (i = 0; i < 15; ++i) {
        if (z >= 0) {
            nx = x - (y >> i);
            y = y + (x >> i);
            x = nx;
            z -= atan_table[i];
        } else {
            nx = x + (y >> i);
            y = y - (x >> i);
            x = nx;
            z += atan_table[i];
        }
    }

    if (out_sine != 0) {
        *out_sine = sin_sign < 0 ? -y : y;
    }
    if (out_cosine != 0) {
        *out_cosine = cos_sign < 0 ? -x : x;
    }
}

g3d_fx g3d_sin(g3d_angle angle)
{
    g3d_fx s;
    g3d_sincos(angle, &s, 0);
    return s;
}

g3d_fx g3d_cos(g3d_angle angle)
{
    g3d_fx c;
    g3d_sincos(angle, 0, &c);
    return c;
}

g3d_vec2 g3d_vec2_make(g3d_fx x, g3d_fx y)
{
    g3d_vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

g3d_vec3 g3d_vec3_make(g3d_fx x, g3d_fx y, g3d_fx z)
{
    g3d_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

g3d_vec3 g3d_vec3_add(g3d_vec3 a, g3d_vec3 b)
{
    return g3d_vec3_make(g3d_fx_add(a.x, b.x),
                         g3d_fx_add(a.y, b.y),
                         g3d_fx_add(a.z, b.z));
}

g3d_vec3 g3d_vec3_sub(g3d_vec3 a, g3d_vec3 b)
{
    return g3d_vec3_make(g3d_fx_sub(a.x, b.x),
                         g3d_fx_sub(a.y, b.y),
                         g3d_fx_sub(a.z, b.z));
}

g3d_vec3 g3d_vec3_scale(g3d_vec3 v, g3d_fx scalar)
{
    return g3d_vec3_make(g3d_fx_mul(v.x, scalar),
                         g3d_fx_mul(v.y, scalar),
                         g3d_fx_mul(v.z, scalar));
}

g3d_fx g3d_vec3_dot(g3d_vec3 a, g3d_vec3 b)
{
    g3d_fx result;
    result = g3d_fx_mul(a.x, b.x);
    result = g3d_fx_add(result, g3d_fx_mul(a.y, b.y));
    result = g3d_fx_add(result, g3d_fx_mul(a.z, b.z));
    return result;
}

g3d_vec3 g3d_vec3_cross(g3d_vec3 a, g3d_vec3 b)
{
    g3d_vec3 result;
    result.x = g3d_fx_sub(g3d_fx_mul(a.y, b.z), g3d_fx_mul(a.z, b.y));
    result.y = g3d_fx_sub(g3d_fx_mul(a.z, b.x), g3d_fx_mul(a.x, b.z));
    result.z = g3d_fx_sub(g3d_fx_mul(a.x, b.y), g3d_fx_mul(a.y, b.x));
    return result;
}

g3d_fx g3d_vec3_length_sq(g3d_vec3 v)
{
    return g3d_vec3_dot(v, v);
}

g3d_fx g3d_vec3_length(g3d_vec3 v)
{
    return g3d_fx_sqrt(g3d_vec3_length_sq(v));
}

g3d_vec3 g3d_vec3_normalize(g3d_vec3 v)
{
    g3d_fx length;
    length = g3d_vec3_length(v);
    if (length <= G3D_FX_EPSILON) {
        return g3d_vec3_make(0, 0, 0);
    }
    return g3d_vec3_scale(v, g3d_fx_div(G3D_FX_ONE, length));
}

g3d_vec3 g3d_vec3_lerp(g3d_vec3 a, g3d_vec3 b, g3d_fx t)
{
    return g3d_vec3_make(g3d_fx_lerp(a.x, b.x, t),
                         g3d_fx_lerp(a.y, b.y, t),
                         g3d_fx_lerp(a.z, b.z, t));
}

g3d_color g3d_color_rgba(unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a)
{
    g3d_color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

g3d_color g3d_color_lerp(g3d_color a, g3d_color b, g3d_fx t)
{
    g3d_color result;
    long value;
    value = (long)a.r + g3d_fx_to_int(g3d_fx_mul(g3d_fx_from_int((long)b.r - (long)a.r), t));
    result.r = (unsigned char)g3d_fx_to_int(g3d_fx_clamp(g3d_fx_from_int(value), 0, g3d_fx_from_int(255)));
    value = (long)a.g + g3d_fx_to_int(g3d_fx_mul(g3d_fx_from_int((long)b.g - (long)a.g), t));
    result.g = (unsigned char)g3d_fx_to_int(g3d_fx_clamp(g3d_fx_from_int(value), 0, g3d_fx_from_int(255)));
    value = (long)a.b + g3d_fx_to_int(g3d_fx_mul(g3d_fx_from_int((long)b.b - (long)a.b), t));
    result.b = (unsigned char)g3d_fx_to_int(g3d_fx_clamp(g3d_fx_from_int(value), 0, g3d_fx_from_int(255)));
    value = (long)a.a + g3d_fx_to_int(g3d_fx_mul(g3d_fx_from_int((long)b.a - (long)a.a), t));
    result.a = (unsigned char)g3d_fx_to_int(g3d_fx_clamp(g3d_fx_from_int(value), 0, g3d_fx_from_int(255)));
    return result;
}

int g3d_mesh_init(g3d_mesh *mesh,
                  g3d_vertex *vertices, unsigned short vertex_capacity,
                  g3d_index *indices, unsigned short index_capacity)
{
    if (mesh == 0 || vertices == 0 || indices == 0) {
        return G3D_ERR_NULL;
    }
    mesh->vertices = vertices;
    mesh->indices = indices;
    mesh->vertex_capacity = vertex_capacity;
    mesh->index_capacity = index_capacity;
    mesh->vertex_count = 0U;
    mesh->index_count = 0U;
    mesh->primitive = G3D_PRIMITIVE_TRIANGLES;
    mesh->error = G3D_OK;
    return G3D_OK;
}

void g3d_mesh_clear(g3d_mesh *mesh)
{
    if (mesh == 0) {
        return;
    }
    mesh->vertex_count = 0U;
    mesh->index_count = 0U;
    mesh->primitive = G3D_PRIMITIVE_TRIANGLES;
    mesh->error = G3D_OK;
}

int g3d_mesh_add_vertex(g3d_mesh *mesh, const g3d_vertex *vertex,
                        g3d_index *out_index)
{
    if (mesh == 0 || vertex == 0) {
        return G3D_ERR_NULL;
    }
    if (mesh->vertex_count >= mesh->vertex_capacity) {
        mesh->error = G3D_ERR_CAPACITY;
        return G3D_ERR_CAPACITY;
    }
    mesh->vertices[mesh->vertex_count] = *vertex;
    if (out_index != 0) {
        *out_index = mesh->vertex_count;
    }
    mesh->vertex_count = (unsigned short)(mesh->vertex_count + 1U);
    return G3D_OK;
}

int g3d_mesh_add_triangle(g3d_mesh *mesh, g3d_index a,
                          g3d_index b, g3d_index c)
{
    if (mesh == 0) {
        return G3D_ERR_NULL;
    }
    if ((unsigned long)mesh->index_count + 3UL > (unsigned long)mesh->index_capacity) {
        mesh->error = G3D_ERR_CAPACITY;
        return G3D_ERR_CAPACITY;
    }
    mesh->primitive = G3D_PRIMITIVE_TRIANGLES;
    mesh->indices[mesh->index_count++] = a;
    mesh->indices[mesh->index_count++] = b;
    mesh->indices[mesh->index_count++] = c;
    return G3D_OK;
}

int g3d_mesh_add_line(g3d_mesh *mesh, g3d_index a, g3d_index b)
{
    if (mesh == 0) {
        return G3D_ERR_NULL;
    }
    if ((unsigned long)mesh->index_count + 2UL > (unsigned long)mesh->index_capacity) {
        mesh->error = G3D_ERR_CAPACITY;
        return G3D_ERR_CAPACITY;
    }
    mesh->primitive = G3D_PRIMITIVE_LINES;
    mesh->indices[mesh->index_count++] = a;
    mesh->indices[mesh->index_count++] = b;
    return G3D_OK;
}

int g3d_mesh_compute_aabb(const g3d_mesh *mesh, g3d_aabb *out_aabb)
{
    unsigned short i;
    if (mesh == 0 || out_aabb == 0) {
        return G3D_ERR_NULL;
    }
    if (mesh->vertex_count == 0U) {
        return G3D_ERR_DEGENERATE;
    }
    out_aabb->min = mesh->vertices[0].position;
    out_aabb->max = mesh->vertices[0].position;
    for (i = 1U; i < mesh->vertex_count; ++i) {
        out_aabb->min.x = g3d_fx_min2(out_aabb->min.x, mesh->vertices[i].position.x);
        out_aabb->min.y = g3d_fx_min2(out_aabb->min.y, mesh->vertices[i].position.y);
        out_aabb->min.z = g3d_fx_min2(out_aabb->min.z, mesh->vertices[i].position.z);
        out_aabb->max.x = g3d_fx_max2(out_aabb->max.x, mesh->vertices[i].position.x);
        out_aabb->max.y = g3d_fx_max2(out_aabb->max.y, mesh->vertices[i].position.y);
        out_aabb->max.z = g3d_fx_max2(out_aabb->max.z, mesh->vertices[i].position.z);
    }
    return G3D_OK;
}

int g3d_mesh_has_capacity(const g3d_mesh *mesh,
                          g3d_mesh_requirements requirements)
{
    if (mesh == 0) {
        return G3D_FALSE;
    }
    return requirements.vertices <= (unsigned long)mesh->vertex_capacity &&
           requirements.indices <= (unsigned long)mesh->index_capacity &&
           requirements.vertices <= 65535UL;
}

static g3d_mesh_requirements g3d_require_make(unsigned long vertices,
                                               unsigned long indices)
{
    g3d_mesh_requirements requirements;
    requirements.vertices = vertices;
    requirements.indices = indices;
    return requirements;
}

g3d_mesh_requirements g3d_require_triangle(void)
{
    return g3d_require_make(3UL, 3UL);
}

g3d_mesh_requirements g3d_require_quad(void)
{
    return g3d_require_make(4UL, 6UL);
}

g3d_mesh_requirements g3d_require_grid(unsigned short segments_x,
                                       unsigned short segments_z)
{
    return g3d_require_make((unsigned long)(segments_x + 1U) *
                            (unsigned long)(segments_z + 1U),
                            (unsigned long)segments_x *
                            (unsigned long)segments_z * 6UL);
}

g3d_mesh_requirements g3d_require_disc(unsigned short segments)
{
    return g3d_require_make((unsigned long)segments + 2UL,
                            (unsigned long)segments * 3UL);
}

g3d_mesh_requirements g3d_require_box(void)
{
    return g3d_require_make(24UL, 36UL);
}

g3d_mesh_requirements g3d_require_prism(unsigned short sides)
{
    return g3d_require_make((unsigned long)sides * 6UL + 2UL,
                            (unsigned long)sides * 12UL);
}

g3d_mesh_requirements g3d_require_pyramid(unsigned short sides)
{
    return g3d_require_make((unsigned long)sides * 4UL + 1UL,
                            (unsigned long)sides * 6UL);
}

g3d_mesh_requirements g3d_require_bipyramid(unsigned short sides)
{
    return g3d_require_make((unsigned long)sides * 6UL,
                            (unsigned long)sides * 6UL);
}

g3d_mesh_requirements g3d_require_frustum(g3d_fx bottom_radius,
                                          g3d_fx top_radius,
                                          unsigned short segments,
                                          int cap_bottom, int cap_top)
{
    unsigned long vertices;
    unsigned long indices;
    vertices = ((unsigned long)segments + 1UL) * 2UL;
    indices = (unsigned long)segments *
              ((bottom_radius == 0 || top_radius == 0) ? 3UL : 6UL);
    if (cap_bottom && bottom_radius > 0) {
        vertices += (unsigned long)segments + 2UL;
        indices += (unsigned long)segments * 3UL;
    }
    if (cap_top && top_radius > 0) {
        vertices += (unsigned long)segments + 2UL;
        indices += (unsigned long)segments * 3UL;
    }
    return g3d_require_make(vertices, indices);
}

g3d_mesh_requirements g3d_require_uv_sphere(unsigned short slices,
                                            unsigned short stacks)
{
    return g3d_require_make((unsigned long)(slices + 1U) *
                            (unsigned long)(stacks + 1U),
                            stacks > 0U ? (unsigned long)slices *
                            (unsigned long)(stacks - 1U) * 6UL : 0UL);
}

g3d_mesh_requirements g3d_require_hemisphere(unsigned short slices,
                                             unsigned short stacks, int cap)
{
    unsigned long vertices;
    unsigned long indices;
    vertices = (unsigned long)(slices + 1U) *
               (unsigned long)(stacks + 1U);
    indices = stacks > 0U ? (unsigned long)slices *
              ((unsigned long)stacks * 6UL - 3UL) : 0UL;
    if (cap) {
        vertices += (unsigned long)slices + 2UL;
        indices += (unsigned long)slices * 3UL;
    }
    return g3d_require_make(vertices, indices);
}

g3d_mesh_requirements g3d_require_capsule(unsigned short slices,
                                          unsigned short hemisphere_stacks)
{
    return g3d_require_make((unsigned long)(slices + 1U) *
                            ((unsigned long)hemisphere_stacks + 1UL) * 2UL,
                            (unsigned long)slices *
                            (unsigned long)hemisphere_stacks * 12UL);
}

void g3d_mesh_set_color(g3d_mesh *mesh, g3d_color color)
{
    unsigned short i;
    if (mesh == 0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].color = color;
    }
}

void g3d_mesh_gradient_y(g3d_mesh *mesh, g3d_fx min_y, g3d_fx max_y,
                         g3d_color bottom, g3d_color top)
{
    unsigned short i;
    g3d_fx range;
    g3d_fx t;
    if (mesh == 0) {
        return;
    }
    range = g3d_fx_sub(max_y, min_y);
    if (range == 0) {
        g3d_mesh_set_color(mesh, bottom);
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        t = g3d_fx_div(g3d_fx_sub(mesh->vertices[i].position.y, min_y), range);
        t = g3d_fx_clamp(t, 0, G3D_FX_ONE);
        mesh->vertices[i].color = g3d_color_lerp(bottom, top, t);
    }
}

void g3d_mesh_checker_uv(g3d_mesh *mesh, unsigned short tiles_u,
                         unsigned short tiles_v, g3d_color a, g3d_color b)
{
    unsigned short i;
    long u;
    long v;
    if (mesh == 0 || tiles_u == 0U || tiles_v == 0U) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        u = g3d_fx_to_int(g3d_fx_mul(mesh->vertices[i].uv.x,
                                     g3d_fx_from_int((long)tiles_u)));
        v = g3d_fx_to_int(g3d_fx_mul(mesh->vertices[i].uv.y,
                                     g3d_fx_from_int((long)tiles_v)));
        mesh->vertices[i].color = ((u + v) & 1L) ? b : a;
    }
}

void g3d_mesh_uv_transform(g3d_mesh *mesh, g3d_fx scale_u,
                           g3d_fx scale_v, g3d_fx offset_u,
                           g3d_fx offset_v)
{
    unsigned short i;
    if (mesh == 0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].uv.x = g3d_fx_add(g3d_fx_mul(mesh->vertices[i].uv.x, scale_u), offset_u);
        mesh->vertices[i].uv.y = g3d_fx_add(g3d_fx_mul(mesh->vertices[i].uv.y, scale_v), offset_v);
    }
}

void g3d_transform_identity(g3d_transform *transform)
{
    if (transform == 0) {
        return;
    }
    transform->position = g3d_vec3_make(0, 0, 0);
    transform->scale = g3d_vec3_make(G3D_FX_ONE, G3D_FX_ONE, G3D_FX_ONE);
    transform->rotation_x = 0U;
    transform->rotation_y = 0U;
    transform->rotation_z = 0U;
}

static g3d_vec3 g3d_rotate_xyz(g3d_vec3 p, g3d_angle rx,
                               g3d_angle ry, g3d_angle rz)
{
    g3d_fx s;
    g3d_fx c;
    g3d_fx x;
    g3d_fx y;
    g3d_fx z;

    g3d_sincos(rx, &s, &c);
    y = g3d_fx_sub(g3d_fx_mul(p.y, c), g3d_fx_mul(p.z, s));
    z = g3d_fx_add(g3d_fx_mul(p.y, s), g3d_fx_mul(p.z, c));
    p.y = y;
    p.z = z;

    g3d_sincos(ry, &s, &c);
    x = g3d_fx_add(g3d_fx_mul(p.x, c), g3d_fx_mul(p.z, s));
    z = g3d_fx_sub(g3d_fx_mul(p.z, c), g3d_fx_mul(p.x, s));
    p.x = x;
    p.z = z;

    g3d_sincos(rz, &s, &c);
    x = g3d_fx_sub(g3d_fx_mul(p.x, c), g3d_fx_mul(p.y, s));
    y = g3d_fx_add(g3d_fx_mul(p.x, s), g3d_fx_mul(p.y, c));
    p.x = x;
    p.y = y;
    return p;
}

g3d_vec3 g3d_transform_point(const g3d_transform *transform, g3d_vec3 point)
{
    if (transform == 0) {
        return point;
    }
    point.x = g3d_fx_mul(point.x, transform->scale.x);
    point.y = g3d_fx_mul(point.y, transform->scale.y);
    point.z = g3d_fx_mul(point.z, transform->scale.z);
    point = g3d_rotate_xyz(point, transform->rotation_x,
                           transform->rotation_y, transform->rotation_z);
    return g3d_vec3_add(point, transform->position);
}

g3d_vec3 g3d_transform_normal(const g3d_transform *transform, g3d_vec3 normal)
{
    if (transform == 0) {
        return normal;
    }
    normal = g3d_rotate_xyz(normal, transform->rotation_x,
                            transform->rotation_y, transform->rotation_z);
    return g3d_vec3_normalize(normal);
}

void g3d_mesh_apply_transform(g3d_mesh *mesh, const g3d_transform *transform)
{
    unsigned short i;
    if (mesh == 0 || transform == 0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].position = g3d_transform_point(transform, mesh->vertices[i].position);
        mesh->vertices[i].normal = g3d_transform_normal(transform, mesh->vertices[i].normal);
    }
}

static g3d_vertex g3d_vertex_make(g3d_vec3 position, g3d_vec3 normal,
                                  g3d_vec2 uv, g3d_color color)
{
    g3d_vertex v;
    v.position = position;
    v.normal = normal;
    v.uv = uv;
    v.color = color;
    return v;
}

static int g3d_validate_mesh(g3d_mesh *mesh)
{
    if (mesh == 0 || mesh->vertices == 0 || mesh->indices == 0) {
        return G3D_ERR_NULL;
    }
    g3d_mesh_clear(mesh);
    return G3D_OK;
}

static int g3d_regular_ring_point(g3d_fx radius, unsigned short index,
                                  unsigned short count, g3d_fx y,
                                  g3d_vec3 *out_point,
                                  g3d_vec3 *out_radial)
{
    g3d_angle angle;
    g3d_fx s;
    g3d_fx c;
    if (count < 3U || out_point == 0) {
        return G3D_ERR_BAD_ARGUMENT;
    }
    angle = (g3d_angle)(((unsigned long)index * 65536UL) / (unsigned long)count);
    g3d_sincos(angle, &s, &c);
    *out_point = g3d_vec3_make(g3d_fx_mul(radius, c), y,
                               g3d_fx_mul(radius, s));
    if (out_radial != 0) {
        *out_radial = g3d_vec3_make(c, 0, s);
    }
    return G3D_OK;
}

int g3d_make_triangle(g3d_mesh *mesh, g3d_fx width, g3d_fx height,
                      g3d_color color)
{
    g3d_fx hw;
    g3d_fx hh;
    g3d_vertex v;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) {
        return rc;
    }
    if (width <= 0 || height <= 0) {
        return G3D_ERR_BAD_ARGUMENT;
    }
    hw = width / 2L;
    hh = height / 2L;
    v = g3d_vertex_make(g3d_vec3_make(-hw, -hh, 0),
                        g3d_vec3_make(0, 0, G3D_FX_ONE),
                        g3d_vec2_make(0, 0), color);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = g3d_vec3_make(hw, -hh, 0);
    v.uv = g3d_vec2_make(G3D_FX_ONE, 0);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = g3d_vec3_make(0, hh, 0);
    v.uv = g3d_vec2_make(G3D_FX_HALF, G3D_FX_ONE);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    return g3d_mesh_add_triangle(mesh, 0U, 1U, 2U);
}

int g3d_make_quad(g3d_mesh *mesh, g3d_fx width, g3d_fx depth,
                  g3d_color color)
{
    g3d_fx hw;
    g3d_fx hd;
    g3d_vertex v;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (width <= 0 || depth <= 0) return G3D_ERR_BAD_ARGUMENT;
    hw = width / 2L;
    hd = depth / 2L;
    v = g3d_vertex_make(g3d_vec3_make(-hw, 0, -hd),
                        g3d_vec3_make(0, G3D_FX_ONE, 0),
                        g3d_vec2_make(0, 0), color);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = g3d_vec3_make(hw, 0, -hd); v.uv.x = G3D_FX_ONE;
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = g3d_vec3_make(hw, 0, hd); v.uv.y = G3D_FX_ONE;
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = g3d_vec3_make(-hw, 0, hd); v.uv.x = 0;
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    if (g3d_mesh_add_triangle(mesh, 0U, 2U, 1U) != G3D_OK) return mesh->error;
    return g3d_mesh_add_triangle(mesh, 0U, 3U, 2U);
}

int g3d_make_grid(g3d_mesh *mesh, g3d_fx width, g3d_fx depth,
                  unsigned short segments_x, unsigned short segments_z,
                  g3d_color color)
{
    unsigned short x;
    unsigned short z;
    unsigned short stride;
    g3d_fx fx;
    g3d_fx fz;
    g3d_fx px;
    g3d_fx pz;
    g3d_vertex v;
    g3d_index i0;
    g3d_index i1;
    g3d_index i2;
    g3d_index i3;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (width <= 0 || depth <= 0 || segments_x == 0U || segments_z == 0U) {
        return G3D_ERR_BAD_ARGUMENT;
    }
    if ((unsigned long)(segments_x + 1U) * (unsigned long)(segments_z + 1U) > 65535UL) {
        return G3D_ERR_RANGE;
    }
    for (z = 0U; z <= segments_z; ++z) {
        fz = g3d_fx_div(g3d_fx_from_int((long)z), g3d_fx_from_int((long)segments_z));
        pz = g3d_fx_sub(g3d_fx_mul(depth, fz), depth / 2L);
        for (x = 0U; x <= segments_x; ++x) {
            fx = g3d_fx_div(g3d_fx_from_int((long)x), g3d_fx_from_int((long)segments_x));
            px = g3d_fx_sub(g3d_fx_mul(width, fx), width / 2L);
            v = g3d_vertex_make(g3d_vec3_make(px, 0, pz),
                                g3d_vec3_make(0, G3D_FX_ONE, 0),
                                g3d_vec2_make(fx, fz), color);
            if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        }
    }
    stride = (unsigned short)(segments_x + 1U);
    for (z = 0U; z < segments_z; ++z) {
        for (x = 0U; x < segments_x; ++x) {
            i0 = (g3d_index)((unsigned long)z * stride + x);
            i1 = (g3d_index)(i0 + 1U);
            i3 = (g3d_index)((unsigned long)(z + 1U) * stride + x);
            i2 = (g3d_index)(i3 + 1U);
            if (g3d_mesh_add_triangle(mesh, i0, i2, i1) != G3D_OK) return mesh->error;
            if (g3d_mesh_add_triangle(mesh, i0, i3, i2) != G3D_OK) return mesh->error;
        }
    }
    return G3D_OK;
}

int g3d_make_disc(g3d_mesh *mesh, g3d_fx radius,
                  unsigned short segments, g3d_color color)
{
    unsigned short i;
    g3d_vec3 p;
    g3d_vertex v;
    g3d_fx u;
    g3d_fx w;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (radius <= 0 || segments < 3U) return G3D_ERR_BAD_ARGUMENT;
    v = g3d_vertex_make(g3d_vec3_make(0, 0, 0),
                        g3d_vec3_make(0, G3D_FX_ONE, 0),
                        g3d_vec2_make(G3D_FX_HALF, G3D_FX_HALF), color);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    for (i = 0U; i <= segments; ++i) {
        g3d_regular_ring_point(radius, (unsigned short)(i % segments), segments, 0, &p, 0);
        u = g3d_fx_add(G3D_FX_HALF, g3d_fx_div(p.x, g3d_fx_mul(radius, g3d_fx_from_int(2))));
        w = g3d_fx_add(G3D_FX_HALF, g3d_fx_div(p.z, g3d_fx_mul(radius, g3d_fx_from_int(2))));
        v = g3d_vertex_make(p, g3d_vec3_make(0, G3D_FX_ONE, 0),
                            g3d_vec2_make(u, w), color);
        if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    }
    for (i = 0U; i < segments; ++i) {
        if (g3d_mesh_add_triangle(mesh, 0U, (g3d_index)(i + 2U),
                                  (g3d_index)(i + 1U)) != G3D_OK) return mesh->error;
    }
    return G3D_OK;
}

static int g3d_add_box_face(g3d_mesh *mesh,
                            g3d_vec3 p0, g3d_vec3 p1,
                            g3d_vec3 p2, g3d_vec3 p3,
                            g3d_vec3 normal, g3d_color color)
{
    g3d_vertex v;
    g3d_index base;
    base = mesh->vertex_count;
    v = g3d_vertex_make(p0, normal, g3d_vec2_make(0, 0), color);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = p1; v.uv = g3d_vec2_make(G3D_FX_ONE, 0);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = p2; v.uv = g3d_vec2_make(G3D_FX_ONE, G3D_FX_ONE);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    v.position = p3; v.uv = g3d_vec2_make(0, G3D_FX_ONE);
    if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    if (g3d_mesh_add_triangle(mesh, base, (g3d_index)(base + 1U),
                              (g3d_index)(base + 2U)) != G3D_OK) return mesh->error;
    return g3d_mesh_add_triangle(mesh, base, (g3d_index)(base + 2U),
                                 (g3d_index)(base + 3U));
}

int g3d_make_box(g3d_mesh *mesh, g3d_fx width, g3d_fx height,
                 g3d_fx depth, g3d_color color)
{
    g3d_fx x;
    g3d_fx y;
    g3d_fx z;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (width <= 0 || height <= 0 || depth <= 0) return G3D_ERR_BAD_ARGUMENT;
    x = width / 2L; y = height / 2L; z = depth / 2L;
    rc = g3d_add_box_face(mesh,
        g3d_vec3_make(-x,-y, z), g3d_vec3_make( x,-y, z),
        g3d_vec3_make( x, y, z), g3d_vec3_make(-x, y, z),
        g3d_vec3_make(0,0,G3D_FX_ONE), color);
    if (rc != G3D_OK) return rc;
    rc = g3d_add_box_face(mesh,
        g3d_vec3_make( x,-y,-z), g3d_vec3_make(-x,-y,-z),
        g3d_vec3_make(-x, y,-z), g3d_vec3_make( x, y,-z),
        g3d_vec3_make(0,0,-G3D_FX_ONE), color);
    if (rc != G3D_OK) return rc;
    rc = g3d_add_box_face(mesh,
        g3d_vec3_make(-x,-y,-z), g3d_vec3_make(-x,-y, z),
        g3d_vec3_make(-x, y, z), g3d_vec3_make(-x, y,-z),
        g3d_vec3_make(-G3D_FX_ONE,0,0), color);
    if (rc != G3D_OK) return rc;
    rc = g3d_add_box_face(mesh,
        g3d_vec3_make( x,-y, z), g3d_vec3_make( x,-y,-z),
        g3d_vec3_make( x, y,-z), g3d_vec3_make( x, y, z),
        g3d_vec3_make(G3D_FX_ONE,0,0), color);
    if (rc != G3D_OK) return rc;
    rc = g3d_add_box_face(mesh,
        g3d_vec3_make(-x, y, z), g3d_vec3_make( x, y, z),
        g3d_vec3_make( x, y,-z), g3d_vec3_make(-x, y,-z),
        g3d_vec3_make(0,G3D_FX_ONE,0), color);
    if (rc != G3D_OK) return rc;
    return g3d_add_box_face(mesh,
        g3d_vec3_make(-x,-y,-z), g3d_vec3_make( x,-y,-z),
        g3d_vec3_make( x,-y, z), g3d_vec3_make(-x,-y, z),
        g3d_vec3_make(0,-G3D_FX_ONE,0), color);
}

int g3d_make_frustum(g3d_mesh *mesh, g3d_fx bottom_radius,
                     g3d_fx top_radius, g3d_fx height,
                     unsigned short segments, int cap_bottom,
                     int cap_top, g3d_color color)
{
    unsigned short i;
    g3d_fx half;
    g3d_fx u;
    g3d_vec3 pb;
    g3d_vec3 pt;
    g3d_vec3 radial;
    g3d_vec3 normal;
    g3d_vertex v;
    g3d_index side_base;
    g3d_index center;
    g3d_index ring_base;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (height <= 0 || bottom_radius < 0 || top_radius < 0 ||
        (bottom_radius == 0 && top_radius == 0) || segments < 3U) {
        return G3D_ERR_BAD_ARGUMENT;
    }
    half = height / 2L;
    side_base = mesh->vertex_count;
    for (i = 0U; i <= segments; ++i) {
        u = g3d_fx_div(g3d_fx_from_int((long)i), g3d_fx_from_int((long)segments));
        g3d_regular_ring_point(bottom_radius, (unsigned short)(i % segments),
                               segments, -half, &pb, &radial);
        g3d_regular_ring_point(top_radius, (unsigned short)(i % segments),
                               segments, half, &pt, 0);
        normal = g3d_vec3_make(g3d_fx_mul(radial.x, height),
                               g3d_fx_sub(bottom_radius, top_radius),
                               g3d_fx_mul(radial.z, height));
        normal = g3d_vec3_normalize(normal);
        v = g3d_vertex_make(pb, normal, g3d_vec2_make(u, 0), color);
        if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        v.position = pt; v.uv.y = G3D_FX_ONE;
        if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
    }
    for (i = 0U; i < segments; ++i) {
        g3d_index b0;
        g3d_index t0;
        g3d_index b1;
        g3d_index t1;
        b0 = (g3d_index)(side_base + i * 2U);
        t0 = (g3d_index)(b0 + 1U);
        b1 = (g3d_index)(b0 + 2U);
        t1 = (g3d_index)(b0 + 3U);
        if (top_radius == 0) {
            if (g3d_mesh_add_triangle(mesh, b0, t0, b1) != G3D_OK) return mesh->error;
        } else if (bottom_radius == 0) {
            if (g3d_mesh_add_triangle(mesh, b0, t0, t1) != G3D_OK) return mesh->error;
        } else {
            if (g3d_mesh_add_triangle(mesh, b0, t0, t1) != G3D_OK) return mesh->error;
            if (g3d_mesh_add_triangle(mesh, b0, t1, b1) != G3D_OK) return mesh->error;
        }
    }

    if (cap_bottom && bottom_radius > 0) {
        center = mesh->vertex_count;
        v = g3d_vertex_make(g3d_vec3_make(0,-half,0),
                            g3d_vec3_make(0,-G3D_FX_ONE,0),
                            g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF), color);
        if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        ring_base = mesh->vertex_count;
        for (i = 0U; i <= segments; ++i) {
            g3d_regular_ring_point(bottom_radius, (unsigned short)(i % segments),
                                   segments, -half, &pb, 0);
            v = g3d_vertex_make(pb, g3d_vec3_make(0,-G3D_FX_ONE,0),
                g3d_vec2_make(g3d_fx_add(G3D_FX_HALF, g3d_fx_div(pb.x, g3d_fx_mul(bottom_radius, g3d_fx_from_int(2)))),
                              g3d_fx_add(G3D_FX_HALF, g3d_fx_div(pb.z, g3d_fx_mul(bottom_radius, g3d_fx_from_int(2))))), color);
            if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        }
        for (i = 0U; i < segments; ++i) {
            if (g3d_mesh_add_triangle(mesh, center,
                 (g3d_index)(ring_base + i),
                 (g3d_index)(ring_base + i + 1U)) != G3D_OK) return mesh->error;
        }
    }

    if (cap_top && top_radius > 0) {
        center = mesh->vertex_count;
        v = g3d_vertex_make(g3d_vec3_make(0,half,0),
                            g3d_vec3_make(0,G3D_FX_ONE,0),
                            g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF), color);
        if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        ring_base = mesh->vertex_count;
        for (i = 0U; i <= segments; ++i) {
            g3d_regular_ring_point(top_radius, (unsigned short)(i % segments),
                                   segments, half, &pt, 0);
            v = g3d_vertex_make(pt, g3d_vec3_make(0,G3D_FX_ONE,0),
                g3d_vec2_make(g3d_fx_add(G3D_FX_HALF, g3d_fx_div(pt.x, g3d_fx_mul(top_radius, g3d_fx_from_int(2)))),
                              g3d_fx_add(G3D_FX_HALF, g3d_fx_div(pt.z, g3d_fx_mul(top_radius, g3d_fx_from_int(2))))), color);
            if (g3d_mesh_add_vertex(mesh, &v, 0) != G3D_OK) return mesh->error;
        }
        for (i = 0U; i < segments; ++i) {
            if (g3d_mesh_add_triangle(mesh, center,
                 (g3d_index)(ring_base + i + 1U),
                 (g3d_index)(ring_base + i)) != G3D_OK) return mesh->error;
        }
    }
    return G3D_OK;
}

int g3d_make_cylinder(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                      unsigned short segments, g3d_color color)
{
    return g3d_make_frustum(mesh, radius, radius, height, segments,
                            G3D_TRUE, G3D_TRUE, color);
}

int g3d_make_cone(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                  unsigned short segments, g3d_color color)
{
    return g3d_make_frustum(mesh, radius, 0, height, segments,
                            G3D_TRUE, G3D_FALSE, color);
}

int g3d_make_prism(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                   unsigned short sides, g3d_color color)
{
    unsigned short i;
    g3d_fx half;
    g3d_vec3 p0;
    g3d_vec3 p1;
    g3d_vec3 q0;
    g3d_vec3 q1;
    g3d_vec3 normal;
    g3d_vec3 edge;
    g3d_vertex v;
    g3d_index base;
    int rc;
    rc = g3d_validate_mesh(mesh);
    if (rc != G3D_OK) return rc;
    if (radius <= 0 || height <= 0 || sides < 3U) return G3D_ERR_BAD_ARGUMENT;
    half = height / 2L;
    for (i = 0U; i < sides; ++i) {
        g3d_regular_ring_point(radius, i, sides, -half, &p0, 0);
        g3d_regular_ring_point(radius, (unsigned short)((i + 1U) % sides), sides, -half, &p1, 0);
        q0 = p0; q0.y = half;
        q1 = p1; q1.y = half;
        edge = g3d_vec3_sub(p1, p0);
        normal = g3d_vec3_normalize(g3d_vec3_make(edge.z, 0, -edge.x));
        base = mesh->vertex_count;
        v = g3d_vertex_make(p0, normal, g3d_vec2_make(0,0), color);
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
        v.position=p1; v.uv.x=G3D_FX_ONE;
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
        v.position=q1; v.uv.y=G3D_FX_ONE;
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
        v.position=q0; v.uv.x=0;
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
        if (g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+2U),(g3d_index)(base+1U))!=G3D_OK) return mesh->error;
        if (g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+3U),(g3d_index)(base+2U))!=G3D_OK) return mesh->error;
    }
    base = mesh->vertex_count;
    v = g3d_vertex_make(g3d_vec3_make(0,-half,0),g3d_vec3_make(0,-G3D_FX_ONE,0),g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF),color);
    if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
    for (i=0U;i<sides;++i) {
        g3d_regular_ring_point(radius,i,sides,-half,&p0,0);
        v.position=p0;
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
    }
    for (i=0U;i<sides;++i) {
        if (g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+1U+i),(g3d_index)(base+1U+((i+1U)%sides)))!=G3D_OK) return mesh->error;
    }
    base = mesh->vertex_count;
    v = g3d_vertex_make(g3d_vec3_make(0,half,0),g3d_vec3_make(0,G3D_FX_ONE,0),g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF),color);
    if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
    for (i=0U;i<sides;++i) {
        g3d_regular_ring_point(radius,i,sides,half,&p0,0);
        v.position=p0;
        if (g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK) return mesh->error;
    }
    for (i=0U;i<sides;++i) {
        if (g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+1U+((i+1U)%sides)),(g3d_index)(base+1U+i))!=G3D_OK) return mesh->error;
    }
    return G3D_OK;
}

int g3d_make_pyramid(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                     unsigned short sides, g3d_color color)
{
    unsigned short i;
    g3d_fx half;
    g3d_vec3 p0;
    g3d_vec3 p1;
    g3d_vec3 apex;
    g3d_vec3 normal;
    g3d_vertex v;
    g3d_index base;
    int rc;
    rc=g3d_validate_mesh(mesh);
    if(rc!=G3D_OK)return rc;
    if(radius<=0||height<=0||sides<3U)return G3D_ERR_BAD_ARGUMENT;
    half=height/2L;
    apex=g3d_vec3_make(0,half,0);
    for(i=0U;i<sides;++i){
        g3d_regular_ring_point(radius,i,sides,-half,&p0,0);
        g3d_regular_ring_point(radius,(unsigned short)((i+1U)%sides),sides,-half,&p1,0);
        normal=g3d_vec3_normalize(g3d_vec3_cross(g3d_vec3_sub(apex,p0),g3d_vec3_sub(p1,p0)));
        base=mesh->vertex_count;
        v=g3d_vertex_make(p0,normal,g3d_vec2_make(0,0),color);
        if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
        v.position=p1;v.uv.x=G3D_FX_ONE;
        if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
        v.position=apex;v.uv=g3d_vec2_make(G3D_FX_HALF,G3D_FX_ONE);
        if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
        if(g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+2U),(g3d_index)(base+1U))!=G3D_OK)return mesh->error;
    }
    base=mesh->vertex_count;
    v=g3d_vertex_make(g3d_vec3_make(0,-half,0),g3d_vec3_make(0,-G3D_FX_ONE,0),g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF),color);
    if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
    for(i=0U;i<sides;++i){
        g3d_regular_ring_point(radius,i,sides,-half,&p0,0);v.position=p0;
        if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
    }
    for(i=0U;i<sides;++i){
        if(g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+1U+i),(g3d_index)(base+1U+((i+1U)%sides)))!=G3D_OK)return mesh->error;
    }
    return G3D_OK;
}

int g3d_make_bipyramid(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                       unsigned short sides, g3d_color color)
{
    unsigned short i;
    g3d_fx half;
    g3d_vec3 p0;
    g3d_vec3 p1;
    g3d_vec3 apex;
    g3d_vec3 normal;
    g3d_vertex v;
    g3d_index base;
    int upper;
    int rc;
    rc=g3d_validate_mesh(mesh);
    if(rc!=G3D_OK)return rc;
    if(radius<=0||height<=0||sides<3U)return G3D_ERR_BAD_ARGUMENT;
    half=height/2L;
    for(upper=0;upper<2;++upper){
        apex=g3d_vec3_make(0,upper?half:-half,0);
        for(i=0U;i<sides;++i){
            g3d_regular_ring_point(radius,i,sides,0,&p0,0);
            g3d_regular_ring_point(radius,(unsigned short)((i+1U)%sides),sides,0,&p1,0);
            if(upper){
                normal=g3d_vec3_normalize(g3d_vec3_cross(g3d_vec3_sub(apex,p0),g3d_vec3_sub(p1,p0)));
            }else{
                normal=g3d_vec3_normalize(g3d_vec3_cross(g3d_vec3_sub(p1,p0),g3d_vec3_sub(apex,p0)));
            }
            base=mesh->vertex_count;
            v=g3d_vertex_make(p0,normal,g3d_vec2_make(0,0),color);
            if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
            v.position=p1;v.uv.x=G3D_FX_ONE;
            if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
            v.position=apex;v.uv=g3d_vec2_make(G3D_FX_HALF,G3D_FX_ONE);
            if(g3d_mesh_add_vertex(mesh,&v,0)!=G3D_OK)return mesh->error;
            if(upper){
                if(g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+2U),(g3d_index)(base+1U))!=G3D_OK)return mesh->error;
            }else{
                if(g3d_mesh_add_triangle(mesh,base,(g3d_index)(base+1U),(g3d_index)(base+2U))!=G3D_OK)return mesh->error;
            }
        }
    }
    return G3D_OK;
}

int g3d_make_uv_sphere(g3d_mesh *mesh, g3d_fx radius,
                       unsigned short slices, unsigned short stacks,
                       g3d_color color)
{
    unsigned short i;
    unsigned short j;
    unsigned short stride;
    g3d_angle theta;
    g3d_angle phi;
    g3d_fx st;
    g3d_fx ct;
    g3d_fx sp;
    g3d_fx cp;
    g3d_fx u;
    g3d_fx vcoord;
    g3d_vec3 normal;
    g3d_vertex vertex;
    g3d_index a;
    g3d_index b;
    g3d_index c;
    g3d_index d;
    int rc;
    rc=g3d_validate_mesh(mesh);
    if(rc!=G3D_OK)return rc;
    if(radius<=0||slices<3U||stacks<2U)return G3D_ERR_BAD_ARGUMENT;
    if((unsigned long)(slices+1U)*(unsigned long)(stacks+1U)>65535UL)return G3D_ERR_RANGE;
    for(j=0U;j<=stacks;++j){
        phi=(g3d_angle)(((unsigned long)j*32768UL)/(unsigned long)stacks);
        g3d_sincos(phi,&sp,&cp);
        vcoord=g3d_fx_div(g3d_fx_from_int((long)j),g3d_fx_from_int((long)stacks));
        for(i=0U;i<=slices;++i){
            theta=(g3d_angle)(((unsigned long)(i%slices)*65536UL)/(unsigned long)slices);
            g3d_sincos(theta,&st,&ct);
            normal=g3d_vec3_make(g3d_fx_mul(sp,ct),cp,g3d_fx_mul(sp,st));
            u=g3d_fx_div(g3d_fx_from_int((long)i),g3d_fx_from_int((long)slices));
            vertex=g3d_vertex_make(g3d_vec3_scale(normal,radius),normal,g3d_vec2_make(u,vcoord),color);
            if(g3d_mesh_add_vertex(mesh,&vertex,0)!=G3D_OK)return mesh->error;
        }
    }
    stride=(unsigned short)(slices+1U);
    for(j=0U;j<stacks;++j){
        for(i=0U;i<slices;++i){
            a=(g3d_index)((unsigned long)j*stride+i);
            b=(g3d_index)(a+1U);
            d=(g3d_index)((unsigned long)(j+1U)*stride+i);
            c=(g3d_index)(d+1U);
            if(j!=0U){if(g3d_mesh_add_triangle(mesh,a,b,d)!=G3D_OK)return mesh->error;}
            if(j+1U!=stacks){if(g3d_mesh_add_triangle(mesh,b,c,d)!=G3D_OK)return mesh->error;}
        }
    }
    return G3D_OK;
}

int g3d_make_hemisphere(g3d_mesh *mesh, g3d_fx radius,
                        unsigned short slices, unsigned short stacks,
                        int upper, int cap, g3d_color color)
{
    unsigned short i;
    unsigned short j;
    unsigned short stride;
    g3d_angle theta;
    g3d_angle phi;
    g3d_fx st;
    g3d_fx ct;
    g3d_fx sp;
    g3d_fx cp;
    g3d_fx u;
    g3d_fx vcoord;
    g3d_vec3 normal;
    g3d_vertex vertex;
    g3d_index a;
    g3d_index b;
    g3d_index c;
    g3d_index d;
    g3d_index center;
    g3d_index ring;
    int rc;
    rc=g3d_validate_mesh(mesh);
    if(rc!=G3D_OK)return rc;
    if(radius<=0||slices<3U||stacks<1U)return G3D_ERR_BAD_ARGUMENT;
    for(j=0U;j<=stacks;++j){
        phi=(g3d_angle)(((unsigned long)j*16384UL)/(unsigned long)stacks);
        if(!upper)phi=(g3d_angle)(49152UL+((unsigned long)j*16384UL)/(unsigned long)stacks);
        g3d_sincos(phi,&sp,&cp);
        vcoord=g3d_fx_div(g3d_fx_from_int((long)j),g3d_fx_from_int((long)stacks));
        for(i=0U;i<=slices;++i){
            theta=(g3d_angle)(((unsigned long)(i%slices)*65536UL)/(unsigned long)slices);
            g3d_sincos(theta,&st,&ct);
            normal=g3d_vec3_make(g3d_fx_mul(cp,ct),sp,g3d_fx_mul(cp,st));
            u=g3d_fx_div(g3d_fx_from_int((long)i),g3d_fx_from_int((long)slices));
            vertex=g3d_vertex_make(g3d_vec3_scale(normal,radius),normal,g3d_vec2_make(u,vcoord),color);
            if(g3d_mesh_add_vertex(mesh,&vertex,0)!=G3D_OK)return mesh->error;
        }
    }
    stride=(unsigned short)(slices+1U);
    for(j=0U;j<stacks;++j){
        for(i=0U;i<slices;++i){
            a=(g3d_index)((unsigned long)j*stride+i);
            b=(g3d_index)(a+1U);
            d=(g3d_index)((unsigned long)(j+1U)*stride+i);
            c=(g3d_index)(d+1U);
            if(upper || j != 0U){
                if(g3d_mesh_add_triangle(mesh,a,d,b)!=G3D_OK)return mesh->error;
            }
            if(!upper || j + 1U != stacks){
                if(g3d_mesh_add_triangle(mesh,b,d,c)!=G3D_OK)return mesh->error;
            }
        }
    }
    if(cap){
        center=mesh->vertex_count;
        vertex=g3d_vertex_make(g3d_vec3_make(0,0,0),g3d_vec3_make(0,upper?-G3D_FX_ONE:G3D_FX_ONE,0),g3d_vec2_make(G3D_FX_HALF,G3D_FX_HALF),color);
        if(g3d_mesh_add_vertex(mesh,&vertex,0)!=G3D_OK)return mesh->error;
        ring=mesh->vertex_count;
        for(i=0U;i<=slices;++i){
            theta=(g3d_angle)(((unsigned long)(i%slices)*65536UL)/(unsigned long)slices);
            g3d_sincos(theta,&st,&ct);
            vertex.position=g3d_vec3_make(g3d_fx_mul(radius,ct),0,g3d_fx_mul(radius,st));
            vertex.uv=g3d_vec2_make(g3d_fx_add(G3D_FX_HALF,ct/2L),g3d_fx_add(G3D_FX_HALF,st/2L));
            if(g3d_mesh_add_vertex(mesh,&vertex,0)!=G3D_OK)return mesh->error;
        }
        for(i=0U;i<slices;++i){
            if(upper){if(g3d_mesh_add_triangle(mesh,center,(g3d_index)(ring+i),(g3d_index)(ring+i+1U))!=G3D_OK)return mesh->error;}
            else{if(g3d_mesh_add_triangle(mesh,center,(g3d_index)(ring+i+1U),(g3d_index)(ring+i))!=G3D_OK)return mesh->error;}
        }
    }
    return G3D_OK;
}

int g3d_make_capsule(g3d_mesh *mesh, g3d_fx radius,
                     g3d_fx body_height, unsigned short slices,
                     unsigned short hemisphere_stacks, g3d_color color)
{
    unsigned short ring_count;
    unsigned short ring_index;
    unsigned short i;
    unsigned short stride;
    g3d_angle phi;
    g3d_angle theta;
    g3d_fx sp;
    g3d_fx cp;
    g3d_fx st;
    g3d_fx ct;
    g3d_fx center_y;
    g3d_fx y;
    g3d_fx u;
    g3d_fx vcoord;
    g3d_vec3 normal;
    g3d_vec3 position;
    g3d_vertex vertex;
    g3d_index a;
    g3d_index b;
    g3d_index c;
    g3d_index d;
    int upper;
    unsigned short local_ring;
    int rc;
    rc=g3d_validate_mesh(mesh);
    if(rc!=G3D_OK)return rc;
    if(radius<=0||body_height<0||slices<3U||hemisphere_stacks<1U)return G3D_ERR_BAD_ARGUMENT;
    ring_count=(unsigned short)((hemisphere_stacks+1U)*2U);
    if((unsigned long)ring_count*(unsigned long)(slices+1U)>65535UL)return G3D_ERR_RANGE;
    ring_index=0U;
    for(upper=0;upper<2;++upper){
        center_y=upper?body_height/2L:-body_height/2L;
        for(local_ring=0U;local_ring<=hemisphere_stacks;++local_ring){
            if(upper){
                phi=(g3d_angle)(((unsigned long)local_ring*16384UL)/(unsigned long)hemisphere_stacks);
            }else{
                phi=(g3d_angle)(49152UL+((unsigned long)local_ring*16384UL)/(unsigned long)hemisphere_stacks);
            }
            g3d_sincos(phi,&sp,&cp);
            y=g3d_fx_add(center_y,g3d_fx_mul(radius,sp));
            vcoord=g3d_fx_div(g3d_fx_from_int((long)ring_index),g3d_fx_from_int((long)(ring_count-1U)));
            for(i=0U;i<=slices;++i){
                theta=(g3d_angle)(((unsigned long)(i%slices)*65536UL)/(unsigned long)slices);
                g3d_sincos(theta,&st,&ct);
                normal=g3d_vec3_make(g3d_fx_mul(cp,ct),sp,g3d_fx_mul(cp,st));
                position=g3d_vec3_make(g3d_fx_mul(radius,normal.x),y,g3d_fx_mul(radius,normal.z));
                u=g3d_fx_div(g3d_fx_from_int((long)i),g3d_fx_from_int((long)slices));
                vertex=g3d_vertex_make(position,normal,g3d_vec2_make(u,vcoord),color);
                if(g3d_mesh_add_vertex(mesh,&vertex,0)!=G3D_OK)return mesh->error;
            }
            ++ring_index;
        }
    }
    stride=(unsigned short)(slices+1U);
    for(ring_index=0U;ring_index+1U<ring_count;++ring_index){
        for(i=0U;i<slices;++i){
            a=(g3d_index)((unsigned long)ring_index*stride+i);
            b=(g3d_index)(a+1U);
            d=(g3d_index)((unsigned long)(ring_index+1U)*stride+i);
            c=(g3d_index)(d+1U);
            if(ring_index!=0U){if(g3d_mesh_add_triangle(mesh,a,d,b)!=G3D_OK)return mesh->error;}
            if(ring_index+2U!=ring_count){if(g3d_mesh_add_triangle(mesh,b,d,c)!=G3D_OK)return mesh->error;}
        }
    }
    return G3D_OK;
}

g3d_aabb g3d_aabb_from_center_half(g3d_vec3 center, g3d_vec3 half_size)
{
    g3d_aabb box;
    box.min=g3d_vec3_sub(center,half_size);
    box.max=g3d_vec3_add(center,half_size);
    return box;
}

int g3d_point_in_aabb(g3d_vec3 point, const g3d_aabb *box)
{
    if(box==0)return G3D_FALSE;
    return point.x>=box->min.x&&point.x<=box->max.x&&
           point.y>=box->min.y&&point.y<=box->max.y&&
           point.z>=box->min.z&&point.z<=box->max.z;
}

int g3d_aabb_overlap(const g3d_aabb *a, const g3d_aabb *b)
{
    if(a==0||b==0)return G3D_FALSE;
    return a->min.x<=b->max.x&&a->max.x>=b->min.x&&
           a->min.y<=b->max.y&&a->max.y>=b->min.y&&
           a->min.z<=b->max.z&&a->max.z>=b->min.z;
}

g3d_vec3 g3d_closest_point_aabb(g3d_vec3 point, const g3d_aabb *box)
{
    if(box==0)return point;
    point.x=g3d_fx_clamp(point.x,box->min.x,box->max.x);
    point.y=g3d_fx_clamp(point.y,box->min.y,box->max.y);
    point.z=g3d_fx_clamp(point.z,box->min.z,box->max.z);
    return point;
}

static int g3d_vec3_within_radius(g3d_vec3 delta, g3d_fx radius)
{
    g3d_vec3 normalized;
    g3d_fx sum;
    if (radius < 0) {
        return G3D_FALSE;
    }
    if (radius == 0) {
        return delta.x == 0 && delta.y == 0 && delta.z == 0;
    }
    if (g3d_fx_abs(delta.x) > radius ||
        g3d_fx_abs(delta.y) > radius ||
        g3d_fx_abs(delta.z) > radius) {
        return G3D_FALSE;
    }
    normalized.x = g3d_fx_div(delta.x, radius);
    normalized.y = g3d_fx_div(delta.y, radius);
    normalized.z = g3d_fx_div(delta.z, radius);
    sum = g3d_fx_mul(normalized.x, normalized.x);
    sum = g3d_fx_add(sum, g3d_fx_mul(normalized.y, normalized.y));
    sum = g3d_fx_add(sum, g3d_fx_mul(normalized.z, normalized.z));
    return sum <= G3D_FX_ONE;
}

int g3d_sphere_overlap(const g3d_sphere *a, const g3d_sphere *b)
{
    g3d_vec3 d;
    g3d_fx r;
    if(a==0||b==0)return G3D_FALSE;
    d=g3d_vec3_sub(a->center,b->center);
    r=g3d_fx_add(a->radius,b->radius);
    return g3d_vec3_within_radius(d,r);
}

int g3d_sphere_aabb_overlap(const g3d_sphere *sphere, const g3d_aabb *box)
{
    g3d_vec3 p;
    g3d_vec3 d;
    if(sphere==0||box==0)return G3D_FALSE;
    p=g3d_closest_point_aabb(sphere->center,box);
    d=g3d_vec3_sub(sphere->center,p);
    return g3d_vec3_within_radius(d,sphere->radius);
}

g3d_vec3 g3d_closest_point_segment(g3d_vec3 point,
                                   g3d_vec3 a, g3d_vec3 b,
                                   g3d_fx *out_t)
{
    g3d_vec3 ab;
    g3d_fx denom;
    g3d_fx t;
    ab=g3d_vec3_sub(b,a);
    denom=g3d_vec3_dot(ab,ab);
    if(denom<=G3D_FX_EPSILON)t=0;
    else t=g3d_fx_div(g3d_vec3_dot(g3d_vec3_sub(point,a),ab),denom);
    t=g3d_fx_clamp(t,0,G3D_FX_ONE);
    if(out_t!=0)*out_t=t;
    return g3d_vec3_add(a,g3d_vec3_scale(ab,t));
}

int g3d_point_in_capsule(g3d_vec3 point, const g3d_capsule *capsule)
{
    g3d_vec3 p;
    g3d_vec3 d;
    if(capsule==0)return G3D_FALSE;
    p=g3d_closest_point_segment(point,capsule->a,capsule->b,0);
    d=g3d_vec3_sub(point,p);
    return g3d_vec3_within_radius(d,capsule->radius);
}

int g3d_capsule_sphere_overlap(const g3d_capsule *capsule,
                               const g3d_sphere *sphere)
{
    g3d_vec3 p;
    g3d_vec3 d;
    g3d_fx r;
    if(capsule==0||sphere==0)return G3D_FALSE;
    p=g3d_closest_point_segment(sphere->center,capsule->a,capsule->b,0);
    d=g3d_vec3_sub(sphere->center,p);
    r=g3d_fx_add(capsule->radius,sphere->radius);
    return g3d_vec3_within_radius(d,r);
}

static g3d_vec3 g3d_segment_segment_delta(g3d_vec3 p1,g3d_vec3 q1,
                                             g3d_vec3 p2,g3d_vec3 q2)
{
    g3d_vec3 d1;
    g3d_vec3 d2;
    g3d_vec3 r;
    g3d_vec3 c1;
    g3d_vec3 c2;
    g3d_fx a;
    g3d_fx e;
    g3d_fx f;
    g3d_fx c;
    g3d_fx b;
    g3d_fx denom;
    g3d_fx s;
    g3d_fx t;
    d1=g3d_vec3_sub(q1,p1);
    d2=g3d_vec3_sub(q2,p2);
    r=g3d_vec3_sub(p1,p2);
    a=g3d_vec3_dot(d1,d1);
    e=g3d_vec3_dot(d2,d2);
    f=g3d_vec3_dot(d2,r);
    if(a<=G3D_FX_EPSILON&&e<=G3D_FX_EPSILON){
        return r;
    }
    if(a<=G3D_FX_EPSILON){
        s=0;t=g3d_fx_clamp(g3d_fx_div(f,e),0,G3D_FX_ONE);
    }else{
        c=g3d_vec3_dot(d1,r);
        if(e<=G3D_FX_EPSILON){
            t=0;s=g3d_fx_clamp(g3d_fx_div(-c,a),0,G3D_FX_ONE);
        }else{
            b=g3d_vec3_dot(d1,d2);
            denom=g3d_fx_sub(g3d_fx_mul(a,e),g3d_fx_mul(b,b));
            if(g3d_fx_abs(denom)>G3D_FX_EPSILON){
                s=g3d_fx_clamp(g3d_fx_div(g3d_fx_sub(g3d_fx_mul(b,f),g3d_fx_mul(c,e)),denom),0,G3D_FX_ONE);
            }else s=0;
            t=g3d_fx_div(g3d_fx_add(g3d_fx_mul(b,s),f),e);
            if(t<0){t=0;s=g3d_fx_clamp(g3d_fx_div(-c,a),0,G3D_FX_ONE);}
            else if(t>G3D_FX_ONE){t=G3D_FX_ONE;s=g3d_fx_clamp(g3d_fx_div(g3d_fx_sub(b,c),a),0,G3D_FX_ONE);}
        }
    }
    c1=g3d_vec3_add(p1,g3d_vec3_scale(d1,s));
    c2=g3d_vec3_add(p2,g3d_vec3_scale(d2,t));
    return g3d_vec3_sub(c1,c2);
}

int g3d_capsule_overlap(const g3d_capsule *a, const g3d_capsule *b)
{
    g3d_fx r;
    g3d_aabb box_a;
    g3d_aabb box_b;
    g3d_vec3 delta;
    if(a==0||b==0)return G3D_FALSE;
    box_a.min=g3d_vec3_make(g3d_fx_sub(g3d_fx_min2(a->a.x,a->b.x),a->radius),
                             g3d_fx_sub(g3d_fx_min2(a->a.y,a->b.y),a->radius),
                             g3d_fx_sub(g3d_fx_min2(a->a.z,a->b.z),a->radius));
    box_a.max=g3d_vec3_make(g3d_fx_add(g3d_fx_max2(a->a.x,a->b.x),a->radius),
                             g3d_fx_add(g3d_fx_max2(a->a.y,a->b.y),a->radius),
                             g3d_fx_add(g3d_fx_max2(a->a.z,a->b.z),a->radius));
    box_b.min=g3d_vec3_make(g3d_fx_sub(g3d_fx_min2(b->a.x,b->b.x),b->radius),
                             g3d_fx_sub(g3d_fx_min2(b->a.y,b->b.y),b->radius),
                             g3d_fx_sub(g3d_fx_min2(b->a.z,b->b.z),b->radius));
    box_b.max=g3d_vec3_make(g3d_fx_add(g3d_fx_max2(b->a.x,b->b.x),b->radius),
                             g3d_fx_add(g3d_fx_max2(b->a.y,b->b.y),b->radius),
                             g3d_fx_add(g3d_fx_max2(b->a.z,b->b.z),b->radius));
    if(!g3d_aabb_overlap(&box_a,&box_b))return G3D_FALSE;
    r=g3d_fx_add(a->radius,b->radius);
    delta=g3d_segment_segment_delta(a->a,a->b,b->a,b->b);
    return g3d_vec3_within_radius(delta,r);
}

g3d_fx g3d_plane_distance(const g3d_plane *plane, g3d_vec3 point)
{
    if(plane==0)return G3D_FX_MAX;
    return g3d_fx_add(g3d_vec3_dot(plane->normal,point),plane->d);
}

int g3d_sphere_plane_overlap(const g3d_sphere *sphere,
                             const g3d_plane *plane)
{
    if(sphere==0||plane==0)return G3D_FALSE;
    return g3d_fx_abs(g3d_plane_distance(plane,sphere->center))<=sphere->radius;
}

static void g3d_hit_clear(g3d_hit *hit)
{
    if(hit==0)return;
    hit->hit=G3D_FALSE;
    hit->t=G3D_FX_MAX;
    hit->point=g3d_vec3_make(0,0,0);
    hit->normal=g3d_vec3_make(0,0,0);
    hit->u=0;
    hit->v=0;
}

int g3d_ray_plane(const g3d_ray *ray, const g3d_plane *plane,
                  g3d_hit *out_hit)
{
    g3d_fx denom;
    g3d_fx t;
    if(ray==0||plane==0||out_hit==0)return G3D_ERR_NULL;
    g3d_hit_clear(out_hit);
    denom=g3d_vec3_dot(plane->normal,ray->direction);
    if(g3d_fx_abs(denom)<=G3D_FX_EPSILON)return G3D_FALSE;
    t=g3d_fx_div(-g3d_plane_distance(plane,ray->origin),denom);
    if(t<0)return G3D_FALSE;
    out_hit->hit=G3D_TRUE;out_hit->t=t;
    out_hit->point=g3d_vec3_add(ray->origin,g3d_vec3_scale(ray->direction,t));
    out_hit->normal=denom<0?plane->normal:g3d_vec3_scale(plane->normal,-G3D_FX_ONE);
    return G3D_TRUE;
}

int g3d_ray_sphere(const g3d_ray *ray, const g3d_sphere *sphere,
                   g3d_hit *out_hit)
{
    g3d_vec3 m;
    g3d_fx a;
    g3d_fx b;
    g3d_fx c;
    g3d_fx disc;
    g3d_fx t;
    if(ray==0||sphere==0||out_hit==0)return G3D_ERR_NULL;
    g3d_hit_clear(out_hit);
    m=g3d_vec3_sub(ray->origin,sphere->center);
    a=g3d_vec3_dot(ray->direction,ray->direction);
    if(a<=G3D_FX_EPSILON)return G3D_FALSE;
    b=g3d_vec3_dot(m,ray->direction);
    c=g3d_fx_sub(g3d_vec3_dot(m,m),g3d_fx_mul(sphere->radius,sphere->radius));
    if(c>0&&b>0)return G3D_FALSE;
    disc=g3d_fx_sub(g3d_fx_mul(b,b),g3d_fx_mul(a,c));
    if(disc<0)return G3D_FALSE;
    t=g3d_fx_div(g3d_fx_sub(-b,g3d_fx_sqrt(disc)),a);
    if(t<0)t=0;
    out_hit->hit=G3D_TRUE;out_hit->t=t;
    out_hit->point=g3d_vec3_add(ray->origin,g3d_vec3_scale(ray->direction,t));
    out_hit->normal=g3d_vec3_normalize(g3d_vec3_sub(out_hit->point,sphere->center));
    return G3D_TRUE;
}

static int g3d_ray_aabb_axis(g3d_fx origin,g3d_fx direction,
                             g3d_fx minv,g3d_fx maxv,
                             g3d_fx *tmin,g3d_fx *tmax)
{
    g3d_fx t1;
    g3d_fx t2;
    g3d_fx temp;
    if(g3d_fx_abs(direction)<=G3D_FX_EPSILON){
        return origin>=minv&&origin<=maxv;
    }
    t1=g3d_fx_div(g3d_fx_sub(minv,origin),direction);
    t2=g3d_fx_div(g3d_fx_sub(maxv,origin),direction);
    if(t1>t2){temp=t1;t1=t2;t2=temp;}
    if(t1>*tmin)*tmin=t1;
    if(t2<*tmax)*tmax=t2;
    return *tmin<=*tmax;
}

int g3d_ray_aabb(const g3d_ray *ray, const g3d_aabb *box,
                 g3d_hit *out_hit)
{
    g3d_fx tmin;
    g3d_fx tmax;
    g3d_vec3 p;
    g3d_vec3 n;
    g3d_fx eps;
    if(ray==0||box==0||out_hit==0)return G3D_ERR_NULL;
    g3d_hit_clear(out_hit);
    tmin=0;tmax=G3D_FX_MAX;
    if(!g3d_ray_aabb_axis(ray->origin.x,ray->direction.x,box->min.x,box->max.x,&tmin,&tmax))return G3D_FALSE;
    if(!g3d_ray_aabb_axis(ray->origin.y,ray->direction.y,box->min.y,box->max.y,&tmin,&tmax))return G3D_FALSE;
    if(!g3d_ray_aabb_axis(ray->origin.z,ray->direction.z,box->min.z,box->max.z,&tmin,&tmax))return G3D_FALSE;
    if(tmax<0)return G3D_FALSE;
    if(tmin<0)tmin=tmax;
    p=g3d_vec3_add(ray->origin,g3d_vec3_scale(ray->direction,tmin));
    n=g3d_vec3_make(0,0,0);eps=32L;
    if(g3d_fx_abs(g3d_fx_sub(p.x,box->min.x))<=eps)n.x=-G3D_FX_ONE;
    else if(g3d_fx_abs(g3d_fx_sub(p.x,box->max.x))<=eps)n.x=G3D_FX_ONE;
    else if(g3d_fx_abs(g3d_fx_sub(p.y,box->min.y))<=eps)n.y=-G3D_FX_ONE;
    else if(g3d_fx_abs(g3d_fx_sub(p.y,box->max.y))<=eps)n.y=G3D_FX_ONE;
    else if(g3d_fx_abs(g3d_fx_sub(p.z,box->min.z))<=eps)n.z=-G3D_FX_ONE;
    else n.z=G3D_FX_ONE;
    out_hit->hit=G3D_TRUE;out_hit->t=tmin;out_hit->point=p;out_hit->normal=n;
    return G3D_TRUE;
}

int g3d_ray_triangle(const g3d_ray *ray,
                     g3d_vec3 a, g3d_vec3 b, g3d_vec3 c,
                     g3d_hit *out_hit)
{
    g3d_vec3 edge1;
    g3d_vec3 edge2;
    g3d_vec3 pvec;
    g3d_vec3 tvec;
    g3d_vec3 qvec;
    g3d_fx det;
    g3d_fx inv_det;
    g3d_fx u;
    g3d_fx v;
    g3d_fx t;
    g3d_vec3 normal;
    if(ray==0||out_hit==0)return G3D_ERR_NULL;
    g3d_hit_clear(out_hit);
    edge1=g3d_vec3_sub(b,a);
    edge2=g3d_vec3_sub(c,a);
    pvec=g3d_vec3_cross(ray->direction,edge2);
    det=g3d_vec3_dot(edge1,pvec);
    if(g3d_fx_abs(det)<=G3D_FX_EPSILON)return G3D_FALSE;
    inv_det=g3d_fx_div(G3D_FX_ONE,det);
    tvec=g3d_vec3_sub(ray->origin,a);
    u=g3d_fx_mul(g3d_vec3_dot(tvec,pvec),inv_det);
    if(u<0||u>G3D_FX_ONE)return G3D_FALSE;
    qvec=g3d_vec3_cross(tvec,edge1);
    v=g3d_fx_mul(g3d_vec3_dot(ray->direction,qvec),inv_det);
    if(v<0||g3d_fx_add(u,v)>G3D_FX_ONE)return G3D_FALSE;
    t=g3d_fx_mul(g3d_vec3_dot(edge2,qvec),inv_det);
    if(t<0)return G3D_FALSE;
    normal=g3d_vec3_normalize(g3d_vec3_cross(edge1,edge2));
    if(g3d_vec3_dot(normal,ray->direction)>0)normal=g3d_vec3_scale(normal,-G3D_FX_ONE);
    out_hit->hit=G3D_TRUE;out_hit->t=t;out_hit->u=u;out_hit->v=v;
    out_hit->point=g3d_vec3_add(ray->origin,g3d_vec3_scale(ray->direction,t));
    out_hit->normal=normal;
    return G3D_TRUE;
}

int g3d_ray_mesh(const g3d_ray *ray, const g3d_mesh *mesh,
                 g3d_hit *out_hit, unsigned short *out_triangle)
{
    unsigned short i;
    unsigned short tri;
    g3d_index ia;
    g3d_index ib;
    g3d_index ic;
    g3d_hit hit;
    int found;
    if(ray==0||mesh==0||out_hit==0)return G3D_ERR_NULL;
    g3d_hit_clear(out_hit);
    if(mesh->primitive!=G3D_PRIMITIVE_TRIANGLES)return G3D_ERR_BAD_ARGUMENT;
    found=G3D_FALSE;tri=0U;
    for(i=0U;i+2U<mesh->index_count;i=(unsigned short)(i+3U),++tri){
        ia=mesh->indices[i];ib=mesh->indices[i+1U];ic=mesh->indices[i+2U];
        if(ia>=mesh->vertex_count||ib>=mesh->vertex_count||ic>=mesh->vertex_count)continue;
        if(g3d_ray_triangle(ray,mesh->vertices[ia].position,mesh->vertices[ib].position,mesh->vertices[ic].position,&hit)==G3D_TRUE){
            if(!found||hit.t<out_hit->t){*out_hit=hit;found=G3D_TRUE;if(out_triangle!=0)*out_triangle=tri;}
        }
    }
    return found;
}
