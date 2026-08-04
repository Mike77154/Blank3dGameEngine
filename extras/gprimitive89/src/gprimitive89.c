#include "gprimitive89.h"

/* C89 compile-time assumptions. */
typedef char gp89_int_must_be_32_bits[(sizeof(int) == 4) ? 1 : -1];
typedef char gp89_short_must_be_16_bits[(sizeof(unsigned short) == 2) ? 1 : -1];

typedef unsigned int gp89_u32;

#define GP89_FX_MAX 2147483647
#define GP89_FX_MIN (-2147483647 - 1)
#define GP89_CORDIC_K 39797
#define GP89_PHI 106039
#define GP89_INV_PHI 40503

static const int gp89_cordic_angles[16] = {
    8192, 4836, 2555, 1297, 651, 326, 163, 81,
    41, 20, 10, 5, 3, 1, 1, 0
};

static gp89_fx gp89_abs_fx(gp89_fx value)
{
    if (value == GP89_FX_MIN) {
        return GP89_FX_MAX;
    }
    return value < 0 ? -value : value;
}

static gp89_u32 gp89_abs_u32(gp89_fx value)
{
    if (value < 0) {
        return (gp89_u32)(-(value + 1)) + 1U;
    }
    return (gp89_u32)value;
}

static gp89_fx gp89_saturate_signed(gp89_u32 magnitude, int negative)
{
    if (negative) {
        if (magnitude >= 0x80000000U) {
            return GP89_FX_MIN;
        }
        return -(gp89_fx)magnitude;
    }
    if (magnitude > 0x7fffffffU) {
        return GP89_FX_MAX;
    }
    return (gp89_fx)magnitude;
}

static gp89_fx gp89_arshift(gp89_fx value, unsigned int bits)
{
    gp89_u32 magnitude;
    gp89_u32 round_mask;

    if (bits == 0U) {
        return value;
    }
    if (bits >= 31U) {
        return value < 0 ? -1 : 0;
    }
    if (value >= 0) {
        return value >> bits;
    }
    magnitude = gp89_abs_u32(value);
    round_mask = (1U << bits) - 1U;
    return -(gp89_fx)((magnitude + round_mask) >> bits);
}

static gp89_fx gp89_clamp_fx(gp89_fx value, gp89_fx low, gp89_fx high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static unsigned char gp89_lerp_u8(unsigned char a,
                                  unsigned char b,
                                  gp89_fx t)
{
    int delta;
    int value;

    delta = (int)b - (int)a;
    value = (int)a + (int)gp89_fx_mul(gp89_fx_from_int(delta), t) / GP89_FX_ONE;
    if (value < 0) {
        value = 0;
    }
    if (value > 255) {
        value = 255;
    }
    return (unsigned char)value;
}

static gp89_fx gp89_vec3_length(gp89_vec3 value)
{
    gp89_fx xx;
    gp89_fx yy;
    gp89_fx zz;
    gp89_fx sum;

    xx = gp89_fx_mul(value.x, value.x);
    yy = gp89_fx_mul(value.y, value.y);
    zz = gp89_fx_mul(value.z, value.z);
    sum = xx + yy;
    if ((yy > 0 && sum < xx) || sum < 0) {
        sum = GP89_FX_MAX;
    }
    if (zz > 0 && GP89_FX_MAX - sum < zz) {
        sum = GP89_FX_MAX;
    } else {
        sum += zz;
    }
    return gp89_fx_sqrt(sum);
}

static gp89_vec3 gp89_vec3_sub(gp89_vec3 a, gp89_vec3 b)
{
    gp89_vec3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static gp89_vec3 gp89_vec3_cross(gp89_vec3 a, gp89_vec3 b)
{
    gp89_vec3 result;

    result.x = gp89_fx_mul(a.y, b.z) - gp89_fx_mul(a.z, b.y);
    result.y = gp89_fx_mul(a.z, b.x) - gp89_fx_mul(a.x, b.z);
    result.z = gp89_fx_mul(a.x, b.y) - gp89_fx_mul(a.y, b.x);
    return result;
}

static gp89_vertex gp89_vertex_make(gp89_fx x,
                                    gp89_fx y,
                                    gp89_fx z,
                                    gp89_fx nx,
                                    gp89_fx ny,
                                    gp89_fx nz,
                                    gp89_fx tx,
                                    gp89_fx ty,
                                    gp89_fx tz,
                                    gp89_fx tw,
                                    gp89_fx u,
                                    gp89_fx v)
{
    gp89_vertex out;

    out.x = x;
    out.y = y;
    out.z = z;
    out.nx = nx;
    out.ny = ny;
    out.nz = nz;
    out.tx = tx;
    out.ty = ty;
    out.tz = tz;
    out.tw = tw;
    out.u = u;
    out.v = v;
    out.r = 255U;
    out.g = 255U;
    out.b = 255U;
    out.a = 255U;
    return out;
}

static int gp89_require(gp89_mesh *mesh,
                        unsigned int extra_vertices,
                        unsigned int extra_triangles)
{
    if (mesh == (gp89_mesh *)0 ||
        mesh->vertices == (gp89_vertex *)0 ||
        mesh->triangles == (gp89_triangle *)0) {
        return GP89_ERR_ARGUMENT;
    }
    if (mesh->vertex_count + extra_vertices > mesh->vertex_capacity ||
        mesh->vertex_count + extra_vertices > 65534U) {
        mesh->status = GP89_ERR_VERTEX_CAPACITY;
        return mesh->status;
    }
    if (mesh->triangle_count + extra_triangles > mesh->triangle_capacity) {
        mesh->status = GP89_ERR_TRI_CAPACITY;
        return mesh->status;
    }
    return GP89_OK;
}

static int gp89_add_vertex(gp89_mesh *mesh,
                           gp89_vertex vertex,
                           gp89_index *out_index)
{
    int status;

    status = gp89_require(mesh, 1U, 0U);
    if (status != GP89_OK) {
        return status;
    }
    mesh->vertices[mesh->vertex_count] = vertex;
    if (out_index != (gp89_index *)0) {
        *out_index = (gp89_index)mesh->vertex_count;
    }
    mesh->vertex_count += 1U;
    return GP89_OK;
}

static int gp89_add_triangle(gp89_mesh *mesh,
                             gp89_index a,
                             gp89_index b,
                             gp89_index c,
                             unsigned short material)
{
    int status;
    gp89_triangle *triangle;

    if ((unsigned int)a >= mesh->vertex_count ||
        (unsigned int)b >= mesh->vertex_count ||
        (unsigned int)c >= mesh->vertex_count) {
        mesh->status = GP89_ERR_INDEX_RANGE;
        return mesh->status;
    }
    status = gp89_require(mesh, 0U, 1U);
    if (status != GP89_OK) {
        return status;
    }
    triangle = &mesh->triangles[mesh->triangle_count];
    triangle->a = a;
    triangle->b = b;
    triangle->c = c;
    triangle->material = material;
    mesh->triangle_count += 1U;
    return GP89_OK;
}

static int gp89_add_flat_triangle(gp89_mesh *mesh,
                                  gp89_vec3 a,
                                  gp89_vec3 b,
                                  gp89_vec3 c,
                                  gp89_vec2 uva,
                                  gp89_vec2 uvb,
                                  gp89_vec2 uvc,
                                  unsigned short material)
{
    gp89_vec3 ab;
    gp89_vec3 ac;
    gp89_vec3 normal;
    gp89_vec3 tangent;
    gp89_vertex va;
    gp89_vertex vb;
    gp89_vertex vc;
    gp89_index ia;
    gp89_index ib;
    gp89_index ic;
    int status;

    status = gp89_require(mesh, 3U, 1U);
    if (status != GP89_OK) {
        return status;
    }
    ab = gp89_vec3_sub(b, a);
    ac = gp89_vec3_sub(c, a);
    normal = gp89_vec3_normalized(gp89_vec3_cross(ab, ac));
    tangent = gp89_vec3_normalized(ab);
    va = gp89_vertex_make(a.x, a.y, a.z,
                          normal.x, normal.y, normal.z,
                          tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                          uva.x, uva.y);
    vb = gp89_vertex_make(b.x, b.y, b.z,
                          normal.x, normal.y, normal.z,
                          tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                          uvb.x, uvb.y);
    vc = gp89_vertex_make(c.x, c.y, c.z,
                          normal.x, normal.y, normal.z,
                          tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                          uvc.x, uvc.y);
    gp89_add_vertex(mesh, va, &ia);
    gp89_add_vertex(mesh, vb, &ib);
    gp89_add_vertex(mesh, vc, &ic);
    return gp89_add_triangle(mesh, ia, ib, ic, material);
}

static gp89_vec3 gp89_rotate_vec(gp89_vec3 value,
                                 gp89_angle x,
                                 gp89_angle y,
                                 gp89_angle z)
{
    gp89_fx sx;
    gp89_fx cx;
    gp89_fx sy;
    gp89_fx cy;
    gp89_fx sz;
    gp89_fx cz;
    gp89_fx py;
    gp89_fx pz;
    gp89_fx px;
    gp89_vec3 out;

    gp89_sincos(x, &sx, &cx);
    gp89_sincos(y, &sy, &cy);
    gp89_sincos(z, &sz, &cz);

    py = gp89_fx_mul(value.y, cx) - gp89_fx_mul(value.z, sx);
    pz = gp89_fx_mul(value.y, sx) + gp89_fx_mul(value.z, cx);
    value.y = py;
    value.z = pz;

    px = gp89_fx_mul(value.x, cy) + gp89_fx_mul(value.z, sy);
    pz = -gp89_fx_mul(value.x, sy) + gp89_fx_mul(value.z, cy);
    value.x = px;
    value.z = pz;

    px = gp89_fx_mul(value.x, cz) - gp89_fx_mul(value.y, sz);
    py = gp89_fx_mul(value.x, sz) + gp89_fx_mul(value.y, cz);
    out.x = px;
    out.y = py;
    out.z = value.z;
    return out;
}

static gp89_fx gp89_axis_value(const gp89_vertex *vertex, int axis)
{
    if (axis == GP89_AXIS_X) {
        return vertex->x;
    }
    if (axis == GP89_AXIS_Z) {
        return vertex->z;
    }
    return vertex->y;
}

static void gp89_bounds_axis(const gp89_mesh *mesh,
                             int axis,
                             gp89_fx *out_min,
                             gp89_fx *out_max)
{
    unsigned int i;
    gp89_fx value;
    gp89_fx minimum;
    gp89_fx maximum;

    if (mesh->vertex_count == 0U) {
        *out_min = 0;
        *out_max = 0;
        return;
    }
    minimum = gp89_axis_value(&mesh->vertices[0], axis);
    maximum = minimum;
    for (i = 1U; i < mesh->vertex_count; ++i) {
        value = gp89_axis_value(&mesh->vertices[i], axis);
        if (value < minimum) {
            minimum = value;
        }
        if (value > maximum) {
            maximum = value;
        }
    }
    *out_min = minimum;
    *out_max = maximum;
}

static gp89_vec2 gp89_uv(gp89_fx x, gp89_fx y)
{
    gp89_vec2 out;

    out.x = x;
    out.y = y;
    return out;
}

static gp89_vec3 gp89_v3(gp89_fx x, gp89_fx y, gp89_fx z)
{
    gp89_vec3 out;

    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

/* ------------------------------------------------------------------------- */
/* Fixed-point math.                                                         */
/* ------------------------------------------------------------------------- */

gp89_fx gp89_fx_from_int(int value)
{
    if (value > 32767) {
        return GP89_FX_MAX;
    }
    if (value < -32768) {
        return GP89_FX_MIN;
    }
    return value * GP89_FX_ONE;
}

int gp89_fx_to_int(gp89_fx value)
{
    if (value >= 0) {
        return value / GP89_FX_ONE;
    }
    return -((gp89_abs_fx(value) + GP89_FX_ONE - 1) / GP89_FX_ONE);
}

gp89_fx gp89_fx_mul(gp89_fx a, gp89_fx b)
{
    gp89_u32 ua;
    gp89_u32 ub;
    gp89_u32 ah;
    gp89_u32 al;
    gp89_u32 bh;
    gp89_u32 bl;
    gp89_u32 p0;
    gp89_u32 p1;
    gp89_u32 p2;
    gp89_u32 p3;
    gp89_u32 middle;
    gp89_u32 high;
    gp89_u32 low;
    gp89_u32 result;
    int negative;

    negative = ((a < 0) != (b < 0));
    ua = gp89_abs_u32(a);
    ub = gp89_abs_u32(b);
    ah = ua >> 16;
    al = ua & 0xffffU;
    bh = ub >> 16;
    bl = ub & 0xffffU;
    p0 = al * bl;
    p1 = ah * bl;
    p2 = al * bh;
    p3 = ah * bh;
    middle = (p0 >> 16) + (p1 & 0xffffU) + (p2 & 0xffffU);
    low = (p0 & 0xffffU) | (middle << 16);
    high = p3 + (p1 >> 16) + (p2 >> 16) + (middle >> 16);
    if (high > 0x00007fffU) {
        return negative ? GP89_FX_MIN : GP89_FX_MAX;
    }
    result = (high << 16) | (low >> 16);
    return gp89_saturate_signed(result, negative);
}

gp89_fx gp89_fx_div(gp89_fx a, gp89_fx b)
{
    gp89_u32 numerator_high;
    gp89_u32 numerator_low;
    gp89_u32 denominator;
    gp89_u32 remainder;
    gp89_u32 quotient;
    gp89_u32 bit;
    gp89_u32 carry;
    int i;
    int overflow;
    int negative;

    if (b == 0) {
        return a < 0 ? GP89_FX_MIN : GP89_FX_MAX;
    }
    negative = ((a < 0) != (b < 0));
    numerator_low = gp89_abs_u32(a) << 16;
    numerator_high = gp89_abs_u32(a) >> 16;
    denominator = gp89_abs_u32(b);
    remainder = 0U;
    quotient = 0U;
    overflow = 0;
    for (i = 47; i >= 0; --i) {
        if (i >= 32) {
            bit = (numerator_high >> (i - 32)) & 1U;
        } else {
            bit = (numerator_low >> i) & 1U;
        }
        carry = remainder >> 31;
        remainder = (remainder << 1) | bit;
        if (carry != 0U || remainder >= denominator) {
            remainder -= denominator;
            if (i >= 32) {
                overflow = 1;
            } else {
                quotient |= 1U << i;
            }
        }
    }
    if (overflow) {
        return negative ? GP89_FX_MIN : GP89_FX_MAX;
    }
    return gp89_saturate_signed(quotient, negative);
}

gp89_fx gp89_fx_sqrt(gp89_fx value)
{
    gp89_fx guess;
    gp89_fx next;
    int i;

    if (value <= 0) {
        return 0;
    }
    guess = value > GP89_FX_ONE ? value : GP89_FX_ONE;
    for (i = 0; i < 20; ++i) {
        next = (guess + gp89_fx_div(value, guess)) / 2;
        if (gp89_abs_fx(next - guess) <= 1) {
            break;
        }
        guess = next;
    }
    return guess;
}

gp89_fx gp89_fx_lerp(gp89_fx a, gp89_fx b, gp89_fx t)
{
    return a + gp89_fx_mul(b - a, t);
}

void gp89_sincos(gp89_angle angle, gp89_fx *out_sin, gp89_fx *out_cos)
{
    int z;
    int i;
    int sign;
    gp89_fx x;
    gp89_fx y;
    gp89_fx x_next;
    gp89_fx y_next;
    gp89_fx x_shift;
    gp89_fx y_shift;

    z = (int)angle;
    if (z >= 32768) {
        z -= 65536;
    }
    sign = 1;
    if (z > 16384) {
        z -= 32768;
        sign = -1;
    } else if (z < -16384) {
        z += 32768;
        sign = -1;
    }
    x = GP89_CORDIC_K;
    y = 0;
    for (i = 0; i < 16; ++i) {
        x_shift = gp89_arshift(x, (unsigned int)i);
        y_shift = gp89_arshift(y, (unsigned int)i);
        if (z >= 0) {
            x_next = x - y_shift;
            y_next = y + x_shift;
            z -= gp89_cordic_angles[i];
        } else {
            x_next = x + y_shift;
            y_next = y - x_shift;
            z += gp89_cordic_angles[i];
        }
        x = x_next;
        y = y_next;
    }
    if (out_sin != (gp89_fx *)0) {
        *out_sin = sign > 0 ? y : -y;
    }
    if (out_cos != (gp89_fx *)0) {
        *out_cos = sign > 0 ? x : -x;
    }
}

gp89_vec3 gp89_vec3_normalized(gp89_vec3 value)
{
    gp89_fx length;
    gp89_vec3 result;

    length = gp89_vec3_length(value);
    if (length <= 0) {
        result.x = 0;
        result.y = GP89_FX_ONE;
        result.z = 0;
        return result;
    }
    result.x = gp89_fx_div(value.x, length);
    result.y = gp89_fx_div(value.y, length);
    result.z = gp89_fx_div(value.z, length);
    return result;
}

/* ------------------------------------------------------------------------- */
/* Mesh lifecycle.                                                           */
/* ------------------------------------------------------------------------- */

void gp89_mesh_init(gp89_mesh *mesh,
                    gp89_vertex *vertex_buffer,
                    unsigned int vertex_capacity,
                    gp89_triangle *triangle_buffer,
                    unsigned int triangle_capacity)
{
    if (mesh == (gp89_mesh *)0) {
        return;
    }
    mesh->vertices = vertex_buffer;
    mesh->triangles = triangle_buffer;
    mesh->vertex_capacity = vertex_capacity;
    mesh->triangle_capacity = triangle_capacity;
    mesh->vertex_count = 0U;
    mesh->triangle_count = 0U;
    mesh->status = GP89_OK;
}

void gp89_mesh_reset(gp89_mesh *mesh)
{
    if (mesh == (gp89_mesh *)0) {
        return;
    }
    mesh->vertex_count = 0U;
    mesh->triangle_count = 0U;
    mesh->status = GP89_OK;
}

int gp89_mesh_valid(const gp89_mesh *mesh)
{
    if (mesh == (const gp89_mesh *)0) {
        return 0;
    }
    return mesh->status == GP89_OK;
}

/* ------------------------------------------------------------------------- */
/* Surface primitives.                                                       */
/* ------------------------------------------------------------------------- */

int gp89_make_triangle(gp89_mesh *mesh, gp89_fx width, gp89_fx height)
{
    gp89_fx hw;
    gp89_fx hh;
    gp89_vec3 a;
    gp89_vec3 b;
    gp89_vec3 c;

    if (mesh == (gp89_mesh *)0 || width <= 0 || height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    hw = width / 2;
    hh = height / 2;
    a = gp89_v3(-hw, -hh, 0);
    b = gp89_v3(hw, -hh, 0);
    c = gp89_v3(0, hh, 0);
    return gp89_add_flat_triangle(mesh, a, b, c,
                                  gp89_uv(0, 0),
                                  gp89_uv(GP89_FX_ONE, 0),
                                  gp89_uv(GP89_FX_HALF, GP89_FX_ONE),
                                  0U);
}

int gp89_make_quad(gp89_mesh *mesh, gp89_fx width, gp89_fx height)
{
    gp89_fx hw;
    gp89_fx hh;
    gp89_vertex vertices[4];
    gp89_index indices[4];
    int i;
    int status;

    if (mesh == (gp89_mesh *)0 || width <= 0 || height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, 4U, 2U);
    if (status != GP89_OK) {
        return status;
    }
    hw = width / 2;
    hh = height / 2;
    vertices[0] = gp89_vertex_make(-hw, -hh, 0,
                                    0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                    0, 0);
    vertices[1] = gp89_vertex_make(hw, -hh, 0,
                                    0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, 0);
    vertices[2] = gp89_vertex_make(hw, hh, 0,
                                    0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, GP89_FX_ONE);
    vertices[3] = gp89_vertex_make(-hw, hh, 0,
                                    0, 0, GP89_FX_ONE,
                                    GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                    0, GP89_FX_ONE);
    for (i = 0; i < 4; ++i) {
        gp89_add_vertex(mesh, vertices[i], &indices[i]);
    }
    gp89_add_triangle(mesh, indices[0], indices[1], indices[2], 0U);
    return gp89_add_triangle(mesh, indices[0], indices[2], indices[3], 0U);
}

int gp89_make_plane(gp89_mesh *mesh,
                    gp89_fx width,
                    gp89_fx depth,
                    unsigned int x_segments,
                    unsigned int z_segments)
{
    unsigned int x;
    unsigned int z;
    unsigned int row;
    unsigned int base;
    gp89_fx fx;
    gp89_fx fz;
    gp89_fx px;
    gp89_fx pz;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || width <= 0 || depth <= 0 ||
        x_segments == 0U || z_segments == 0U) {
        return GP89_ERR_ARGUMENT;
    }
    if ((x_segments + 1U) * (z_segments + 1U) > 65534U) {
        return GP89_ERR_INDEX_RANGE;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          (x_segments + 1U) * (z_segments + 1U),
                          x_segments * z_segments * 2U);
    if (status != GP89_OK) {
        return status;
    }
    for (z = 0U; z <= z_segments; ++z) {
        fz = gp89_fx_div(gp89_fx_from_int((int)z),
                         gp89_fx_from_int((int)z_segments));
        pz = -depth / 2 + gp89_fx_mul(depth, fz);
        for (x = 0U; x <= x_segments; ++x) {
            fx = gp89_fx_div(gp89_fx_from_int((int)x),
                             gp89_fx_from_int((int)x_segments));
            px = -width / 2 + gp89_fx_mul(width, fx);
            vertex = gp89_vertex_make(px, 0, pz,
                                       0, GP89_FX_ONE, 0,
                                       GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                       fx, fz);
            gp89_add_vertex(mesh, vertex, &index);
        }
    }
    row = x_segments + 1U;
    for (z = 0U; z < z_segments; ++z) {
        for (x = 0U; x < x_segments; ++x) {
            base = z * row + x;
            gp89_add_triangle(mesh,
                              (gp89_index)base,
                              (gp89_index)(base + row),
                              (gp89_index)(base + 1U),
                              0U);
            gp89_add_triangle(mesh,
                              (gp89_index)(base + 1U),
                              (gp89_index)(base + row),
                              (gp89_index)(base + row + 1U),
                              0U);
        }
    }
    return mesh->status;
}

int gp89_make_disc(gp89_mesh *mesh, gp89_fx radius, unsigned int segments)
{
    unsigned int i;
    gp89_fx s;
    gp89_fx c;
    gp89_fx u;
    gp89_fx v;
    gp89_angle angle;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || radius <= 0 || segments < 3U) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, segments + 2U, segments);
    if (status != GP89_OK) {
        return status;
    }
    vertex = gp89_vertex_make(0, 0, 0,
                               0, GP89_FX_ONE, 0,
                               GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                               GP89_FX_HALF, GP89_FX_HALF);
    gp89_add_vertex(mesh, vertex, &index);
    for (i = 0U; i <= segments; ++i) {
        angle = (gp89_angle)((i * GP89_TURN) / segments);
        gp89_sincos(angle, &s, &c);
        u = GP89_FX_HALF + gp89_fx_mul(c, GP89_FX_HALF);
        v = GP89_FX_HALF + gp89_fx_mul(s, GP89_FX_HALF);
        vertex = gp89_vertex_make(gp89_fx_mul(radius, c),
                                   0,
                                   gp89_fx_mul(radius, s),
                                   0, GP89_FX_ONE, 0,
                                   -s, 0, c, GP89_FX_ONE,
                                   u, v);
        gp89_add_vertex(mesh, vertex, &index);
    }
    for (i = 0U; i < segments; ++i) {
        gp89_add_triangle(mesh, 0U,
                          (gp89_index)(i + 1U),
                          (gp89_index)(i + 2U),
                          0U);
    }
    return mesh->status;
}

int gp89_make_annulus(gp89_mesh *mesh,
                      gp89_fx inner_radius,
                      gp89_fx outer_radius,
                      unsigned int segments)
{
    unsigned int i;
    gp89_fx s;
    gp89_fx c;
    gp89_fx u;
    gp89_fx v;
    gp89_angle angle;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || inner_radius <= 0 ||
        outer_radius <= inner_radius || segments < 3U) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, (segments + 1U) * 2U, segments * 2U);
    if (status != GP89_OK) {
        return status;
    }
    for (i = 0U; i <= segments; ++i) {
        angle = (gp89_angle)((i * GP89_TURN) / segments);
        gp89_sincos(angle, &s, &c);
        u = GP89_FX_HALF + gp89_fx_mul(c, gp89_fx_div(inner_radius, outer_radius) / 2);
        v = GP89_FX_HALF + gp89_fx_mul(s, gp89_fx_div(inner_radius, outer_radius) / 2);
        vertex = gp89_vertex_make(gp89_fx_mul(inner_radius, c),
                                   0,
                                   gp89_fx_mul(inner_radius, s),
                                   0, GP89_FX_ONE, 0,
                                   -s, 0, c, GP89_FX_ONE,
                                   u, v);
        gp89_add_vertex(mesh, vertex, &index);
        u = GP89_FX_HALF + gp89_fx_mul(c, GP89_FX_HALF);
        v = GP89_FX_HALF + gp89_fx_mul(s, GP89_FX_HALF);
        vertex = gp89_vertex_make(gp89_fx_mul(outer_radius, c),
                                   0,
                                   gp89_fx_mul(outer_radius, s),
                                   0, GP89_FX_ONE, 0,
                                   -s, 0, c, GP89_FX_ONE,
                                   u, v);
        gp89_add_vertex(mesh, vertex, &index);
    }
    for (i = 0U; i < segments; ++i) {
        gp89_index inner0;
        gp89_index outer0;
        gp89_index inner1;
        gp89_index outer1;

        inner0 = (gp89_index)(i * 2U);
        outer0 = (gp89_index)(inner0 + 1U);
        inner1 = (gp89_index)(inner0 + 2U);
        outer1 = (gp89_index)(inner0 + 3U);
        gp89_add_triangle(mesh, inner0, outer0, outer1, 0U);
        gp89_add_triangle(mesh, inner0, outer1, inner1, 0U);
    }
    return mesh->status;
}

/* ------------------------------------------------------------------------- */
/* Rounded and rotational solids.                                            */
/* ------------------------------------------------------------------------- */

int gp89_make_frustum(gp89_mesh *mesh,
                      gp89_fx bottom_radius,
                      gp89_fx top_radius,
                      gp89_fx height,
                      unsigned int slices,
                      int caps)
{
    unsigned int i;
    gp89_fx s;
    gp89_fx c;
    gp89_fx u;
    gp89_fx half_height;
    gp89_fx slope;
    gp89_vec3 normal;
    gp89_vertex vertex;
    gp89_index index;
    unsigned int side_vertices;
    unsigned int side_triangles;
    unsigned int cap_vertices;
    unsigned int cap_triangles;
    gp89_index center;
    gp89_index ring_start;
    gp89_angle angle;
    int status;

    if (mesh == (gp89_mesh *)0 || bottom_radius < 0 || top_radius < 0 ||
        (bottom_radius == 0 && top_radius == 0) || height <= 0 || slices < 3U) {
        return GP89_ERR_ARGUMENT;
    }
    side_vertices = (slices + 1U) * 2U;
    side_triangles = slices * 2U;
    if (top_radius == 0 || bottom_radius == 0) {
        side_triangles = slices;
    }
    cap_vertices = 0U;
    cap_triangles = 0U;
    if ((caps & GP89_CAP_BOTTOM) != 0 && bottom_radius > 0) {
        cap_vertices += slices + 2U;
        cap_triangles += slices;
    }
    if ((caps & GP89_CAP_TOP) != 0 && top_radius > 0) {
        cap_vertices += slices + 2U;
        cap_triangles += slices;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          side_vertices + cap_vertices,
                          side_triangles + cap_triangles);
    if (status != GP89_OK) {
        return status;
    }
    half_height = height / 2;
    slope = gp89_fx_div(bottom_radius - top_radius, height);
    for (i = 0U; i <= slices; ++i) {
        angle = (gp89_angle)((i * GP89_TURN) / slices);
        gp89_sincos(angle, &s, &c);
        normal = gp89_vec3_normalized(gp89_v3(c, slope, s));
        u = gp89_fx_div(gp89_fx_from_int((int)i),
                        gp89_fx_from_int((int)slices));
        vertex = gp89_vertex_make(gp89_fx_mul(bottom_radius, c),
                                   -half_height,
                                   gp89_fx_mul(bottom_radius, s),
                                   normal.x, normal.y, normal.z,
                                   -s, 0, c, GP89_FX_ONE,
                                   u, 0);
        gp89_add_vertex(mesh, vertex, &index);
        vertex = gp89_vertex_make(gp89_fx_mul(top_radius, c),
                                   half_height,
                                   gp89_fx_mul(top_radius, s),
                                   normal.x, normal.y, normal.z,
                                   -s, 0, c, GP89_FX_ONE,
                                   u, GP89_FX_ONE);
        gp89_add_vertex(mesh, vertex, &index);
    }
    for (i = 0U; i < slices; ++i) {
        gp89_index b0;
        gp89_index t0;
        gp89_index b1;
        gp89_index t1;

        b0 = (gp89_index)(i * 2U);
        t0 = (gp89_index)(b0 + 1U);
        b1 = (gp89_index)(b0 + 2U);
        t1 = (gp89_index)(b0 + 3U);
        if (top_radius == 0) {
            gp89_add_triangle(mesh, b0, t1, b1, 0U);
        } else if (bottom_radius == 0) {
            gp89_add_triangle(mesh, b0, t0, t1, 0U);
        } else {
            gp89_add_triangle(mesh, b0, t0, t1, 0U);
            gp89_add_triangle(mesh, b0, t1, b1, 0U);
        }
    }
    if ((caps & GP89_CAP_BOTTOM) != 0 && bottom_radius > 0) {
        vertex = gp89_vertex_make(0, -half_height, 0,
                                   0, -GP89_FX_ONE, 0,
                                   GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                   GP89_FX_HALF, GP89_FX_HALF);
        gp89_add_vertex(mesh, vertex, &center);
        ring_start = (gp89_index)mesh->vertex_count;
        for (i = 0U; i <= slices; ++i) {
            angle = (gp89_angle)((i * GP89_TURN) / slices);
            gp89_sincos(angle, &s, &c);
            vertex = gp89_vertex_make(gp89_fx_mul(bottom_radius, c),
                                       -half_height,
                                       gp89_fx_mul(bottom_radius, s),
                                       0, -GP89_FX_ONE, 0,
                                       GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                       GP89_FX_HALF + gp89_fx_mul(c, GP89_FX_HALF),
                                       GP89_FX_HALF + gp89_fx_mul(s, GP89_FX_HALF));
            gp89_add_vertex(mesh, vertex, &index);
        }
        for (i = 0U; i < slices; ++i) {
            gp89_add_triangle(mesh, center,
                              (gp89_index)(ring_start + i + 1U),
                              (gp89_index)(ring_start + i),
                              1U);
        }
    }
    if ((caps & GP89_CAP_TOP) != 0 && top_radius > 0) {
        vertex = gp89_vertex_make(0, half_height, 0,
                                   0, GP89_FX_ONE, 0,
                                   GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                   GP89_FX_HALF, GP89_FX_HALF);
        gp89_add_vertex(mesh, vertex, &center);
        ring_start = (gp89_index)mesh->vertex_count;
        for (i = 0U; i <= slices; ++i) {
            angle = (gp89_angle)((i * GP89_TURN) / slices);
            gp89_sincos(angle, &s, &c);
            vertex = gp89_vertex_make(gp89_fx_mul(top_radius, c),
                                       half_height,
                                       gp89_fx_mul(top_radius, s),
                                       0, GP89_FX_ONE, 0,
                                       GP89_FX_ONE, 0, 0, GP89_FX_ONE,
                                       GP89_FX_HALF + gp89_fx_mul(c, GP89_FX_HALF),
                                       GP89_FX_HALF + gp89_fx_mul(s, GP89_FX_HALF));
            gp89_add_vertex(mesh, vertex, &index);
        }
        for (i = 0U; i < slices; ++i) {
            gp89_add_triangle(mesh, center,
                              (gp89_index)(ring_start + i),
                              (gp89_index)(ring_start + i + 1U),
                              2U);
        }
    }
    return mesh->status;
}

int gp89_make_cylinder(gp89_mesh *mesh,
                       gp89_fx radius,
                       gp89_fx height,
                       unsigned int slices,
                       int caps)
{
    return gp89_make_frustum(mesh, radius, radius, height, slices, caps);
}

int gp89_make_cone(gp89_mesh *mesh,
                   gp89_fx radius,
                   gp89_fx height,
                   unsigned int slices,
                   int cap)
{
    return gp89_make_frustum(mesh,
                             radius,
                             0,
                             height,
                             slices,
                             cap ? GP89_CAP_BOTTOM : GP89_CAP_NONE);
}

int gp89_make_uv_sphere(gp89_mesh *mesh,
                        gp89_fx radius,
                        unsigned int slices,
                        unsigned int stacks)
{
    unsigned int row;
    unsigned int column;
    unsigned int stride;
    unsigned int base;
    gp89_angle theta;
    gp89_angle phi;
    gp89_fx st;
    gp89_fx ct;
    gp89_fx sp;
    gp89_fx cp;
    gp89_fx u;
    gp89_fx v;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || radius <= 0 || slices < 3U || stacks < 2U) {
        return GP89_ERR_ARGUMENT;
    }
    if ((slices + 1U) * (stacks + 1U) > 65534U) {
        return GP89_ERR_INDEX_RANGE;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          (slices + 1U) * (stacks + 1U),
                          slices * (stacks - 1U) * 2U);
    if (status != GP89_OK) {
        return status;
    }
    for (row = 0U; row <= stacks; ++row) {
        theta = (gp89_angle)((row * GP89_HALF_TURN) / stacks);
        gp89_sincos(theta, &st, &ct);
        v = gp89_fx_div(gp89_fx_from_int((int)row),
                        gp89_fx_from_int((int)stacks));
        for (column = 0U; column <= slices; ++column) {
            phi = (gp89_angle)((column * GP89_TURN) / slices);
            gp89_sincos(phi, &sp, &cp);
            u = gp89_fx_div(gp89_fx_from_int((int)column),
                            gp89_fx_from_int((int)slices));
            vertex = gp89_vertex_make(gp89_fx_mul(radius, gp89_fx_mul(st, cp)),
                                       gp89_fx_mul(radius, ct),
                                       gp89_fx_mul(radius, gp89_fx_mul(st, sp)),
                                       gp89_fx_mul(st, cp), ct, gp89_fx_mul(st, sp),
                                       -sp, 0, cp, GP89_FX_ONE,
                                       u, v);
            gp89_add_vertex(mesh, vertex, &index);
        }
    }
    stride = slices + 1U;
    for (row = 0U; row < stacks; ++row) {
        for (column = 0U; column < slices; ++column) {
            base = row * stride + column;
            if (row != 0U) {
                gp89_add_triangle(mesh,
                                  (gp89_index)base,
                                  (gp89_index)(base + stride),
                                  (gp89_index)(base + 1U),
                                  0U);
            }
            if (row + 1U != stacks) {
                gp89_add_triangle(mesh,
                                  (gp89_index)(base + 1U),
                                  (gp89_index)(base + stride),
                                  (gp89_index)(base + stride + 1U),
                                  0U);
            }
        }
    }
    return mesh->status;
}

int gp89_make_capsule(gp89_mesh *mesh,
                      gp89_fx radius,
                      gp89_fx cylinder_height,
                      unsigned int slices,
                      unsigned int hemisphere_rings)
{
    unsigned int rows;
    unsigned int row;
    unsigned int column;
    unsigned int k;
    unsigned int stride;
    unsigned int base;
    gp89_angle latitude;
    gp89_angle longitude;
    gp89_fx slat;
    gp89_fx clat;
    gp89_fx slon;
    gp89_fx clon;
    gp89_fx radial;
    gp89_fx y;
    gp89_fx ny;
    gp89_fx u;
    gp89_fx v;
    gp89_fx half_cylinder;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || radius <= 0 || cylinder_height < 0 ||
        slices < 3U || hemisphere_rings < 1U) {
        return GP89_ERR_ARGUMENT;
    }
    rows = 2U * (hemisphere_rings + 1U);
    if (rows * (slices + 1U) > 65534U) {
        return GP89_ERR_INDEX_RANGE;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          rows * (slices + 1U),
                          slices * (rows - 2U) * 2U);
    if (status != GP89_OK) {
        return status;
    }
    half_cylinder = cylinder_height / 2;
    for (row = 0U; row < rows; ++row) {
        if (row <= hemisphere_rings) {
            latitude = (gp89_angle)(GP89_QUARTER_TURN -
                       (row * GP89_QUARTER_TURN) / hemisphere_rings);
            gp89_sincos(latitude, &slat, &clat);
            radial = gp89_fx_mul(radius, clat);
            y = half_cylinder + gp89_fx_mul(radius, slat);
            ny = slat;
        } else {
            k = row - (hemisphere_rings + 1U);
            latitude = (gp89_angle)((k * GP89_QUARTER_TURN) / hemisphere_rings);
            gp89_sincos(latitude, &slat, &clat);
            radial = gp89_fx_mul(radius, clat);
            y = -half_cylinder - gp89_fx_mul(radius, slat);
            ny = -slat;
        }
        v = gp89_fx_div(gp89_fx_from_int((int)row),
                        gp89_fx_from_int((int)(rows - 1U)));
        for (column = 0U; column <= slices; ++column) {
            longitude = (gp89_angle)((column * GP89_TURN) / slices);
            gp89_sincos(longitude, &slon, &clon);
            u = gp89_fx_div(gp89_fx_from_int((int)column),
                            gp89_fx_from_int((int)slices));
            vertex = gp89_vertex_make(gp89_fx_mul(radial, clon),
                                       y,
                                       gp89_fx_mul(radial, slon),
                                       gp89_fx_mul(clat, clon),
                                       ny,
                                       gp89_fx_mul(clat, slon),
                                       -slon, 0, clon, GP89_FX_ONE,
                                       u, v);
            gp89_add_vertex(mesh, vertex, &index);
        }
    }
    stride = slices + 1U;
    for (row = 0U; row + 1U < rows; ++row) {
        for (column = 0U; column < slices; ++column) {
            base = row * stride + column;
            if (row != 0U) {
                gp89_add_triangle(mesh,
                                  (gp89_index)base,
                                  (gp89_index)(base + stride),
                                  (gp89_index)(base + 1U),
                                  0U);
            }
            if (row + 2U != rows) {
                gp89_add_triangle(mesh,
                                  (gp89_index)(base + 1U),
                                  (gp89_index)(base + stride),
                                  (gp89_index)(base + stride + 1U),
                                  0U);
            }
        }
    }
    return mesh->status;
}

int gp89_make_spherocylinder(gp89_mesh *mesh,
                             gp89_fx radius,
                             gp89_fx cylinder_height,
                             unsigned int slices,
                             unsigned int hemisphere_rings)
{
    return gp89_make_capsule(mesh,
                             radius,
                             cylinder_height,
                             slices,
                             hemisphere_rings);
}

int gp89_make_rounded_cylinder(gp89_mesh *mesh,
                               gp89_fx radius,
                               gp89_fx cylinder_height,
                               unsigned int slices,
                               unsigned int hemisphere_rings)
{
    return gp89_make_capsule(mesh,
                             radius,
                             cylinder_height,
                             slices,
                             hemisphere_rings);
}

int gp89_make_torus(gp89_mesh *mesh,
                    gp89_fx major_radius,
                    gp89_fx minor_radius,
                    unsigned int major_segments,
                    unsigned int minor_segments)
{
    unsigned int major;
    unsigned int minor;
    unsigned int stride;
    unsigned int base;
    gp89_angle a;
    gp89_angle b;
    gp89_fx sa;
    gp89_fx ca;
    gp89_fx sb;
    gp89_fx cb;
    gp89_fx ring_radius;
    gp89_fx u;
    gp89_fx v;
    gp89_vertex vertex;
    gp89_index index;
    int status;

    if (mesh == (gp89_mesh *)0 || major_radius <= 0 || minor_radius <= 0 ||
        major_segments < 3U || minor_segments < 3U) {
        return GP89_ERR_ARGUMENT;
    }
    if ((major_segments + 1U) * (minor_segments + 1U) > 65534U) {
        return GP89_ERR_INDEX_RANGE;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          (major_segments + 1U) * (minor_segments + 1U),
                          major_segments * minor_segments * 2U);
    if (status != GP89_OK) {
        return status;
    }
    for (major = 0U; major <= major_segments; ++major) {
        a = (gp89_angle)((major * GP89_TURN) / major_segments);
        gp89_sincos(a, &sa, &ca);
        u = gp89_fx_div(gp89_fx_from_int((int)major),
                        gp89_fx_from_int((int)major_segments));
        for (minor = 0U; minor <= minor_segments; ++minor) {
            b = (gp89_angle)((minor * GP89_TURN) / minor_segments);
            gp89_sincos(b, &sb, &cb);
            v = gp89_fx_div(gp89_fx_from_int((int)minor),
                            gp89_fx_from_int((int)minor_segments));
            ring_radius = major_radius + gp89_fx_mul(minor_radius, cb);
            vertex = gp89_vertex_make(gp89_fx_mul(ring_radius, ca),
                                       gp89_fx_mul(minor_radius, sb),
                                       gp89_fx_mul(ring_radius, sa),
                                       gp89_fx_mul(cb, ca),
                                       sb,
                                       gp89_fx_mul(cb, sa),
                                       -sa, 0, ca, GP89_FX_ONE,
                                       u, v);
            gp89_add_vertex(mesh, vertex, &index);
        }
    }
    stride = minor_segments + 1U;
    for (major = 0U; major < major_segments; ++major) {
        for (minor = 0U; minor < minor_segments; ++minor) {
            base = major * stride + minor;
            gp89_add_triangle(mesh,
                              (gp89_index)base,
                              (gp89_index)(base + stride),
                              (gp89_index)(base + 1U),
                              0U);
            gp89_add_triangle(mesh,
                              (gp89_index)(base + 1U),
                              (gp89_index)(base + stride),
                              (gp89_index)(base + stride + 1U),
                              0U);
        }
    }
    return mesh->status;
}

/* ------------------------------------------------------------------------- */
/* Faceted solids and polyhedron families.                                    */
/* ------------------------------------------------------------------------- */

static int gp89_add_flat_quad(gp89_mesh *mesh,
                              gp89_vec3 a,
                              gp89_vec3 b,
                              gp89_vec3 c,
                              gp89_vec3 d,
                              unsigned short material)
{
    gp89_vec3 ab;
    gp89_vec3 ac;
    gp89_vec3 normal;
    gp89_vec3 tangent;
    gp89_vertex vertex;
    gp89_index ia;
    gp89_index ib;
    gp89_index ic;
    gp89_index id;
    int status;

    status = gp89_require(mesh, 4U, 2U);
    if (status != GP89_OK) {
        return status;
    }
    ab = gp89_vec3_sub(b, a);
    ac = gp89_vec3_sub(c, a);
    normal = gp89_vec3_normalized(gp89_vec3_cross(ab, ac));
    tangent = gp89_vec3_normalized(ab);
    vertex = gp89_vertex_make(a.x, a.y, a.z,
                               normal.x, normal.y, normal.z,
                               tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                               0, 0);
    gp89_add_vertex(mesh, vertex, &ia);
    vertex = gp89_vertex_make(b.x, b.y, b.z,
                               normal.x, normal.y, normal.z,
                               tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                               GP89_FX_ONE, 0);
    gp89_add_vertex(mesh, vertex, &ib);
    vertex = gp89_vertex_make(c.x, c.y, c.z,
                               normal.x, normal.y, normal.z,
                               tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                               GP89_FX_ONE, GP89_FX_ONE);
    gp89_add_vertex(mesh, vertex, &ic);
    vertex = gp89_vertex_make(d.x, d.y, d.z,
                               normal.x, normal.y, normal.z,
                               tangent.x, tangent.y, tangent.z, GP89_FX_ONE,
                               0, GP89_FX_ONE);
    gp89_add_vertex(mesh, vertex, &id);
    gp89_add_triangle(mesh, ia, ib, ic, material);
    return gp89_add_triangle(mesh, ia, ic, id, material);
}

static gp89_fx gp89_dot(gp89_vec3 a, gp89_vec3 b)
{
    return gp89_fx_mul(a.x, b.x) +
           gp89_fx_mul(a.y, b.y) +
           gp89_fx_mul(a.z, b.z);
}

static int gp89_add_outward_triangle(gp89_mesh *mesh,
                                     gp89_vec3 a,
                                     gp89_vec3 b,
                                     gp89_vec3 c,
                                     unsigned short material)
{
    gp89_vec3 normal;
    gp89_vec3 center;
    gp89_vec3 temp;

    normal = gp89_vec3_cross(gp89_vec3_sub(b, a), gp89_vec3_sub(c, a));
    center.x = (a.x + b.x + c.x) / 3;
    center.y = (a.y + b.y + c.y) / 3;
    center.z = (a.z + b.z + c.z) / 3;
    if (gp89_dot(normal, center) < 0) {
        temp = b;
        b = c;
        c = temp;
    }
    return gp89_add_flat_triangle(mesh, a, b, c,
                                  gp89_uv(0, 0),
                                  gp89_uv(GP89_FX_ONE, 0),
                                  gp89_uv(GP89_FX_HALF, GP89_FX_ONE),
                                  material);
}

int gp89_make_box(gp89_mesh *mesh,
                  gp89_fx width,
                  gp89_fx height,
                  gp89_fx depth)
{
    gp89_fx x;
    gp89_fx y;
    gp89_fx z;
    int status;

    if (mesh == (gp89_mesh *)0 || width <= 0 || height <= 0 || depth <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, 24U, 12U);
    if (status != GP89_OK) {
        return status;
    }
    x = width / 2;
    y = height / 2;
    z = depth / 2;
    gp89_add_flat_quad(mesh,
        gp89_v3(-x, -y, z), gp89_v3(x, -y, z),
        gp89_v3(x, y, z), gp89_v3(-x, y, z), 0U);
    gp89_add_flat_quad(mesh,
        gp89_v3(x, -y, -z), gp89_v3(-x, -y, -z),
        gp89_v3(-x, y, -z), gp89_v3(x, y, -z), 1U);
    gp89_add_flat_quad(mesh,
        gp89_v3(x, -y, z), gp89_v3(x, -y, -z),
        gp89_v3(x, y, -z), gp89_v3(x, y, z), 2U);
    gp89_add_flat_quad(mesh,
        gp89_v3(-x, -y, -z), gp89_v3(-x, -y, z),
        gp89_v3(-x, y, z), gp89_v3(-x, y, -z), 3U);
    gp89_add_flat_quad(mesh,
        gp89_v3(-x, y, z), gp89_v3(x, y, z),
        gp89_v3(x, y, -z), gp89_v3(-x, y, -z), 4U);
    gp89_add_flat_quad(mesh,
        gp89_v3(-x, -y, -z), gp89_v3(x, -y, -z),
        gp89_v3(x, -y, z), gp89_v3(-x, -y, z), 5U);
    return mesh->status;
}

int gp89_make_cube(gp89_mesh *mesh, gp89_fx size)
{
    return gp89_make_box(mesh, size, size, size);
}

int gp89_make_prism(gp89_mesh *mesh,
                    unsigned int sides,
                    gp89_fx radius,
                    gp89_fx height,
                    int caps)
{
    unsigned int i;
    gp89_angle a0;
    gp89_angle a1;
    gp89_fx s0;
    gp89_fx c0;
    gp89_fx s1;
    gp89_fx c1;
    gp89_fx h;
    gp89_vec3 b0;
    gp89_vec3 b1;
    gp89_vec3 t0;
    gp89_vec3 t1;
    gp89_vec3 center_bottom;
    gp89_vec3 center_top;
    unsigned int cap_count;
    int status;

    if (mesh == (gp89_mesh *)0 || sides < 3U || radius <= 0 || height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    cap_count = 0U;
    if ((caps & GP89_CAP_BOTTOM) != 0) {
        cap_count += 1U;
    }
    if ((caps & GP89_CAP_TOP) != 0) {
        cap_count += 1U;
    }
    status = gp89_require(mesh,
                          sides * 4U + sides * 3U * cap_count,
                          sides * 2U + sides * cap_count);
    if (status != GP89_OK) {
        return status;
    }
    h = height / 2;
    center_bottom = gp89_v3(0, -h, 0);
    center_top = gp89_v3(0, h, 0);
    for (i = 0U; i < sides; ++i) {
        a0 = (gp89_angle)((i * GP89_TURN) / sides);
        a1 = (gp89_angle)(((i + 1U) * GP89_TURN) / sides);
        gp89_sincos(a0, &s0, &c0);
        gp89_sincos(a1, &s1, &c1);
        b0 = gp89_v3(gp89_fx_mul(radius, c0), -h, gp89_fx_mul(radius, s0));
        b1 = gp89_v3(gp89_fx_mul(radius, c1), -h, gp89_fx_mul(radius, s1));
        t0 = gp89_v3(gp89_fx_mul(radius, c0), h, gp89_fx_mul(radius, s0));
        t1 = gp89_v3(gp89_fx_mul(radius, c1), h, gp89_fx_mul(radius, s1));
        gp89_add_flat_quad(mesh, b0, t0, t1, b1, 0U);
        if ((caps & GP89_CAP_BOTTOM) != 0) {
            gp89_add_flat_triangle(mesh, center_bottom, b1, b0,
                                   gp89_uv(GP89_FX_HALF, GP89_FX_HALF),
                                   gp89_uv(GP89_FX_ONE, GP89_FX_ONE),
                                   gp89_uv(0, GP89_FX_ONE), 1U);
        }
        if ((caps & GP89_CAP_TOP) != 0) {
            gp89_add_flat_triangle(mesh, center_top, t0, t1,
                                   gp89_uv(GP89_FX_HALF, GP89_FX_HALF),
                                   gp89_uv(0, 0),
                                   gp89_uv(GP89_FX_ONE, 0), 2U);
        }
    }
    return mesh->status;
}

int gp89_make_pyramid(gp89_mesh *mesh,
                      unsigned int sides,
                      gp89_fx radius,
                      gp89_fx height,
                      int cap)
{
    unsigned int i;
    gp89_angle a0;
    gp89_angle a1;
    gp89_fx s0;
    gp89_fx c0;
    gp89_fx s1;
    gp89_fx c1;
    gp89_fx h;
    gp89_vec3 b0;
    gp89_vec3 b1;
    gp89_vec3 apex;
    gp89_vec3 center;
    int status;

    if (mesh == (gp89_mesh *)0 || sides < 3U || radius <= 0 || height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh,
                          sides * 3U + (cap ? sides * 3U : 0U),
                          sides + (cap ? sides : 0U));
    if (status != GP89_OK) {
        return status;
    }
    h = height / 2;
    apex = gp89_v3(0, h, 0);
    center = gp89_v3(0, -h, 0);
    for (i = 0U; i < sides; ++i) {
        a0 = (gp89_angle)((i * GP89_TURN) / sides);
        a1 = (gp89_angle)(((i + 1U) * GP89_TURN) / sides);
        gp89_sincos(a0, &s0, &c0);
        gp89_sincos(a1, &s1, &c1);
        b0 = gp89_v3(gp89_fx_mul(radius, c0), -h, gp89_fx_mul(radius, s0));
        b1 = gp89_v3(gp89_fx_mul(radius, c1), -h, gp89_fx_mul(radius, s1));
        gp89_add_outward_triangle(mesh, b0, apex, b1, 0U);
        if (cap) {
            gp89_add_flat_triangle(mesh, center, b1, b0,
                                   gp89_uv(GP89_FX_HALF, GP89_FX_HALF),
                                   gp89_uv(GP89_FX_ONE, GP89_FX_ONE),
                                   gp89_uv(0, GP89_FX_ONE), 1U);
        }
    }
    return mesh->status;
}

int gp89_make_antiprism(gp89_mesh *mesh,
                        unsigned int sides,
                        gp89_fx radius,
                        gp89_fx height,
                        int caps)
{
    unsigned int i;
    gp89_angle ab0;
    gp89_angle ab1;
    gp89_angle at0;
    gp89_angle at1;
    gp89_fx sb0;
    gp89_fx cb0;
    gp89_fx sb1;
    gp89_fx cb1;
    gp89_fx st0;
    gp89_fx ct0;
    gp89_fx st1;
    gp89_fx ct1;
    gp89_fx h;
    gp89_vec3 b0;
    gp89_vec3 b1;
    gp89_vec3 t0;
    gp89_vec3 t1;
    gp89_vec3 center_bottom;
    gp89_vec3 center_top;
    unsigned int cap_count;
    int status;

    if (mesh == (gp89_mesh *)0 || sides < 3U || radius <= 0 || height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    cap_count = 0U;
    if ((caps & GP89_CAP_BOTTOM) != 0) {
        cap_count += 1U;
    }
    if ((caps & GP89_CAP_TOP) != 0) {
        cap_count += 1U;
    }
    status = gp89_require(mesh,
                          sides * 6U + sides * 3U * cap_count,
                          sides * 2U + sides * cap_count);
    if (status != GP89_OK) {
        return status;
    }
    h = height / 2;
    center_bottom = gp89_v3(0, -h, 0);
    center_top = gp89_v3(0, h, 0);
    for (i = 0U; i < sides; ++i) {
        ab0 = (gp89_angle)((i * GP89_TURN) / sides);
        ab1 = (gp89_angle)(((i + 1U) * GP89_TURN) / sides);
        at0 = (gp89_angle)(((i * 2U + 1U) * GP89_TURN) / (sides * 2U));
        at1 = (gp89_angle)((((i + 1U) * 2U + 1U) * GP89_TURN) / (sides * 2U));
        gp89_sincos(ab0, &sb0, &cb0);
        gp89_sincos(ab1, &sb1, &cb1);
        gp89_sincos(at0, &st0, &ct0);
        gp89_sincos(at1, &st1, &ct1);
        b0 = gp89_v3(gp89_fx_mul(radius, cb0), -h, gp89_fx_mul(radius, sb0));
        b1 = gp89_v3(gp89_fx_mul(radius, cb1), -h, gp89_fx_mul(radius, sb1));
        t0 = gp89_v3(gp89_fx_mul(radius, ct0), h, gp89_fx_mul(radius, st0));
        t1 = gp89_v3(gp89_fx_mul(radius, ct1), h, gp89_fx_mul(radius, st1));
        gp89_add_outward_triangle(mesh, b0, t0, b1, 0U);
        gp89_add_outward_triangle(mesh, b1, t0, t1, 0U);
        if ((caps & GP89_CAP_BOTTOM) != 0) {
            gp89_add_outward_triangle(mesh, center_bottom, b1, b0, 1U);
        }
        if ((caps & GP89_CAP_TOP) != 0) {
            gp89_add_outward_triangle(mesh, center_top, t0, t1, 2U);
        }
    }
    return mesh->status;
}

int gp89_make_bipyramid(gp89_mesh *mesh,
                        unsigned int sides,
                        gp89_fx radius,
                        gp89_fx top_height,
                        gp89_fx bottom_height)
{
    unsigned int i;
    gp89_angle a0;
    gp89_angle a1;
    gp89_fx s0;
    gp89_fx c0;
    gp89_fx s1;
    gp89_fx c1;
    gp89_vec3 p0;
    gp89_vec3 p1;
    gp89_vec3 top;
    gp89_vec3 bottom;
    int status;

    if (mesh == (gp89_mesh *)0 || sides < 3U || radius <= 0 ||
        top_height <= 0 || bottom_height <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, sides * 6U, sides * 2U);
    if (status != GP89_OK) {
        return status;
    }
    top = gp89_v3(0, top_height, 0);
    bottom = gp89_v3(0, -bottom_height, 0);
    for (i = 0U; i < sides; ++i) {
        a0 = (gp89_angle)((i * GP89_TURN) / sides);
        a1 = (gp89_angle)(((i + 1U) * GP89_TURN) / sides);
        gp89_sincos(a0, &s0, &c0);
        gp89_sincos(a1, &s1, &c1);
        p0 = gp89_v3(gp89_fx_mul(radius, c0), 0, gp89_fx_mul(radius, s0));
        p1 = gp89_v3(gp89_fx_mul(radius, c1), 0, gp89_fx_mul(radius, s1));
        gp89_add_outward_triangle(mesh, p0, top, p1, 0U);
        gp89_add_outward_triangle(mesh, p1, bottom, p0, 1U);
    }
    return mesh->status;
}

int gp89_make_wedge(gp89_mesh *mesh,
                    gp89_fx width,
                    gp89_fx height,
                    gp89_fx depth)
{
    gp89_fx x;
    gp89_fx y;
    gp89_fx z;
    gp89_vec3 p[6];
    int status;

    if (mesh == (gp89_mesh *)0 || width <= 0 || height <= 0 || depth <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, 24U, 8U);
    if (status != GP89_OK) {
        return status;
    }
    x = width / 2;
    y = height / 2;
    z = depth / 2;
    p[0] = gp89_v3(-x, -y, -z);
    p[1] = gp89_v3(x, -y, -z);
    p[2] = gp89_v3(-x, y, -z);
    p[3] = gp89_v3(-x, -y, z);
    p[4] = gp89_v3(x, -y, z);
    p[5] = gp89_v3(-x, y, z);
    gp89_add_outward_triangle(mesh, p[0], p[2], p[1], 0U);
    gp89_add_outward_triangle(mesh, p[3], p[4], p[5], 1U);
    gp89_add_flat_quad(mesh, p[0], p[3], p[5], p[2], 2U);
    gp89_add_flat_quad(mesh, p[0], p[1], p[4], p[3], 3U);
    gp89_add_flat_quad(mesh, p[2], p[5], p[4], p[1], 4U);
    return mesh->status;
}

#include "gprimitive89_polydata.inc"

int gp89_make_custom_flat(gp89_mesh *mesh,
                          const gp89_vec3 *positions,
                          unsigned int position_count,
                          const gp89_face3 *faces,
                          unsigned int face_count,
                          gp89_fx uniform_scale)
{
    unsigned int i;
    gp89_face3 face;
    gp89_vec3 a;
    gp89_vec3 b;
    gp89_vec3 c;
    int status;

    if (mesh == (gp89_mesh *)0 || positions == (const gp89_vec3 *)0 ||
        faces == (const gp89_face3 *)0 || position_count == 0U ||
        face_count == 0U || uniform_scale <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    gp89_mesh_reset(mesh);
    status = gp89_require(mesh, face_count * 3U, face_count);
    if (status != GP89_OK) {
        return status;
    }
    for (i = 0U; i < face_count; ++i) {
        face = faces[i];
        if ((unsigned int)face.a >= position_count ||
            (unsigned int)face.b >= position_count ||
            (unsigned int)face.c >= position_count) {
            mesh->status = GP89_ERR_INDEX_RANGE;
            return mesh->status;
        }
        a = positions[face.a];
        b = positions[face.b];
        c = positions[face.c];
        a.x = gp89_fx_mul(a.x, uniform_scale);
        a.y = gp89_fx_mul(a.y, uniform_scale);
        a.z = gp89_fx_mul(a.z, uniform_scale);
        b.x = gp89_fx_mul(b.x, uniform_scale);
        b.y = gp89_fx_mul(b.y, uniform_scale);
        b.z = gp89_fx_mul(b.z, uniform_scale);
        c.x = gp89_fx_mul(c.x, uniform_scale);
        c.y = gp89_fx_mul(c.y, uniform_scale);
        c.z = gp89_fx_mul(c.z, uniform_scale);
        status = gp89_add_flat_triangle(mesh, a, b, c,
                                        gp89_uv(0, 0),
                                        gp89_uv(GP89_FX_ONE, 0),
                                        gp89_uv(GP89_FX_HALF, GP89_FX_ONE),
                                        0U);
        if (status != GP89_OK) {
            return status;
        }
    }
    return mesh->status;
}

int gp89_make_regular_polyhedron(gp89_mesh *mesh,
                                 int polyhedron_kind,
                                 gp89_fx radius)
{
    if (radius <= 0) {
        return GP89_ERR_ARGUMENT;
    }
    switch (polyhedron_kind) {
    case GP89_POLY_TETRAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_tetrahedron_vertices, 4U,
            gp89_poly_tetrahedron_faces, 4U, radius);
    case GP89_POLY_OCTAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_octahedron_vertices, 6U,
            gp89_poly_octahedron_faces, 8U, radius);
    case GP89_POLY_DODECAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_dodecahedron_vertices, 20U,
            gp89_poly_dodecahedron_faces, 36U, radius);
    case GP89_POLY_ICOSAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_icosahedron_vertices, 12U,
            gp89_poly_icosahedron_faces, 20U, radius);
    case GP89_POLY_CUBOCTAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_cuboctahedron_vertices, 12U,
            gp89_poly_cuboctahedron_faces, 20U, radius);
    case GP89_POLY_RHOMBIC_DODECAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_rhombic_dodecahedron_vertices, 14U,
            gp89_poly_rhombic_dodecahedron_faces, 24U, radius);
    case GP89_POLY_TRUNCATED_OCTAHEDRON:
        return gp89_make_custom_flat(mesh,
            gp89_poly_truncated_octahedron_vertices, 24U,
            gp89_poly_truncated_octahedron_faces, 44U, radius);
    default:
        return GP89_ERR_ARGUMENT;
    }
}

/* ------------------------------------------------------------------------- */
/* Appearance.                                                               */
/* ------------------------------------------------------------------------- */

void gp89_color_all(gp89_mesh *mesh, gp89_rgba color)
{
    unsigned int i;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].r = color.r;
        mesh->vertices[i].g = color.g;
        mesh->vertices[i].b = color.b;
        mesh->vertices[i].a = color.a;
    }
}

void gp89_color_axis_gradient(gp89_mesh *mesh,
                              int axis,
                              gp89_rgba low,
                              gp89_rgba high)
{
    unsigned int i;
    gp89_fx minimum;
    gp89_fx maximum;
    gp89_fx range;
    gp89_fx t;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0 || mesh->vertex_count == 0U) {
        return;
    }
    gp89_bounds_axis(mesh, axis, &minimum, &maximum);
    range = maximum - minimum;
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        if (range == 0) {
            t = 0;
        } else {
            t = gp89_fx_div(gp89_axis_value(vertex, axis) - minimum, range);
        }
        t = gp89_clamp_fx(t, 0, GP89_FX_ONE);
        vertex->r = gp89_lerp_u8(low.r, high.r, t);
        vertex->g = gp89_lerp_u8(low.g, high.g, t);
        vertex->b = gp89_lerp_u8(low.b, high.b, t);
        vertex->a = gp89_lerp_u8(low.a, high.a, t);
    }
}

void gp89_material_all(gp89_mesh *mesh, unsigned short material)
{
    unsigned int i;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        mesh->triangles[i].material = material;
    }
}

void gp89_material_cycle(gp89_mesh *mesh, unsigned short material_count)
{
    unsigned int i;

    if (mesh == (gp89_mesh *)0 || material_count == 0U) {
        return;
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        mesh->triangles[i].material = (unsigned short)(i % material_count);
    }
}

void gp89_uv_transform(gp89_mesh *mesh,
                       gp89_fx scale_u,
                       gp89_fx scale_v,
                       gp89_fx offset_u,
                       gp89_fx offset_v)
{
    unsigned int i;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        vertex->u = gp89_fx_mul(vertex->u, scale_u) + offset_u;
        vertex->v = gp89_fx_mul(vertex->v, scale_v) + offset_v;
    }
}

void gp89_uv_planar(gp89_mesh *mesh, int axis_u, int axis_v)
{
    unsigned int i;
    gp89_fx min_u;
    gp89_fx max_u;
    gp89_fx min_v;
    gp89_fx max_v;
    gp89_fx range_u;
    gp89_fx range_v;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0 || mesh->vertex_count == 0U) {
        return;
    }
    gp89_bounds_axis(mesh, axis_u, &min_u, &max_u);
    gp89_bounds_axis(mesh, axis_v, &min_v, &max_v);
    range_u = max_u - min_u;
    range_v = max_v - min_v;
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        vertex->u = range_u == 0 ? 0 :
            gp89_fx_div(gp89_axis_value(vertex, axis_u) - min_u, range_u);
        vertex->v = range_v == 0 ? 0 :
            gp89_fx_div(gp89_axis_value(vertex, axis_v) - min_v, range_v);
    }
}

/* ------------------------------------------------------------------------- */
/* Geometry processing.                                                      */
/* ------------------------------------------------------------------------- */

void gp89_translate(gp89_mesh *mesh, gp89_fx x, gp89_fx y, gp89_fx z)
{
    unsigned int i;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].x += x;
        mesh->vertices[i].y += y;
        mesh->vertices[i].z += z;
    }
}

void gp89_scale(gp89_mesh *mesh, gp89_fx x, gp89_fx y, gp89_fx z)
{
    unsigned int i;
    gp89_vertex *vertex;
    gp89_vec3 normal;
    gp89_vec3 tangent;
    gp89_fx ix;
    gp89_fx iy;
    gp89_fx iz;

    if (mesh == (gp89_mesh *)0 || x == 0 || y == 0 || z == 0) {
        return;
    }
    ix = gp89_fx_div(GP89_FX_ONE, x);
    iy = gp89_fx_div(GP89_FX_ONE, y);
    iz = gp89_fx_div(GP89_FX_ONE, z);
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        vertex->x = gp89_fx_mul(vertex->x, x);
        vertex->y = gp89_fx_mul(vertex->y, y);
        vertex->z = gp89_fx_mul(vertex->z, z);
        normal = gp89_v3(gp89_fx_mul(vertex->nx, ix),
                          gp89_fx_mul(vertex->ny, iy),
                          gp89_fx_mul(vertex->nz, iz));
        normal = gp89_vec3_normalized(normal);
        vertex->nx = normal.x;
        vertex->ny = normal.y;
        vertex->nz = normal.z;
        tangent = gp89_v3(gp89_fx_mul(vertex->tx, x),
                           gp89_fx_mul(vertex->ty, y),
                           gp89_fx_mul(vertex->tz, z));
        tangent = gp89_vec3_normalized(tangent);
        vertex->tx = tangent.x;
        vertex->ty = tangent.y;
        vertex->tz = tangent.z;
    }
}

void gp89_rotate_xyz(gp89_mesh *mesh,
                     gp89_angle x,
                     gp89_angle y,
                     gp89_angle z)
{
    unsigned int i;
    gp89_vertex *vertex;
    gp89_vec3 value;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        value = gp89_rotate_vec(gp89_v3(vertex->x, vertex->y, vertex->z), x, y, z);
        vertex->x = value.x;
        vertex->y = value.y;
        vertex->z = value.z;
        value = gp89_rotate_vec(gp89_v3(vertex->nx, vertex->ny, vertex->nz), x, y, z);
        vertex->nx = value.x;
        vertex->ny = value.y;
        vertex->nz = value.z;
        value = gp89_rotate_vec(gp89_v3(vertex->tx, vertex->ty, vertex->tz), x, y, z);
        vertex->tx = value.x;
        vertex->ty = value.y;
        vertex->tz = value.z;
    }
}

void gp89_apply_transform(gp89_mesh *mesh, const gp89_transform *transform)
{
    if (mesh == (gp89_mesh *)0 || transform == (const gp89_transform *)0) {
        return;
    }
    gp89_scale(mesh, transform->sx, transform->sy, transform->sz);
    gp89_rotate_xyz(mesh, transform->rx, transform->ry, transform->rz);
    gp89_translate(mesh, transform->tx, transform->ty, transform->tz);
}

void gp89_reverse_winding(gp89_mesh *mesh)
{
    unsigned int i;
    gp89_index temp;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        temp = mesh->triangles[i].b;
        mesh->triangles[i].b = mesh->triangles[i].c;
        mesh->triangles[i].c = temp;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].nx = -mesh->vertices[i].nx;
        mesh->vertices[i].ny = -mesh->vertices[i].ny;
        mesh->vertices[i].nz = -mesh->vertices[i].nz;
        mesh->vertices[i].tw = -mesh->vertices[i].tw;
    }
}

void gp89_recalculate_smooth_normals(gp89_mesh *mesh)
{
    unsigned int i;
    gp89_triangle triangle;
    gp89_vec3 a;
    gp89_vec3 b;
    gp89_vec3 c;
    gp89_vec3 normal;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].nx = 0;
        mesh->vertices[i].ny = 0;
        mesh->vertices[i].nz = 0;
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        triangle = mesh->triangles[i];
        a = gp89_v3(mesh->vertices[triangle.a].x,
                     mesh->vertices[triangle.a].y,
                     mesh->vertices[triangle.a].z);
        b = gp89_v3(mesh->vertices[triangle.b].x,
                     mesh->vertices[triangle.b].y,
                     mesh->vertices[triangle.b].z);
        c = gp89_v3(mesh->vertices[triangle.c].x,
                     mesh->vertices[triangle.c].y,
                     mesh->vertices[triangle.c].z);
        normal = gp89_vec3_cross(gp89_vec3_sub(b, a), gp89_vec3_sub(c, a));
        vertex = &mesh->vertices[triangle.a];
        vertex->nx += normal.x;
        vertex->ny += normal.y;
        vertex->nz += normal.z;
        vertex = &mesh->vertices[triangle.b];
        vertex->nx += normal.x;
        vertex->ny += normal.y;
        vertex->nz += normal.z;
        vertex = &mesh->vertices[triangle.c];
        vertex->nx += normal.x;
        vertex->ny += normal.y;
        vertex->nz += normal.z;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        normal = gp89_vec3_normalized(gp89_v3(vertex->nx, vertex->ny, vertex->nz));
        vertex->nx = normal.x;
        vertex->ny = normal.y;
        vertex->nz = normal.z;
    }
}

void gp89_recalculate_tangents(gp89_mesh *mesh)
{
    unsigned int i;
    gp89_triangle triangle;
    gp89_vertex *va;
    gp89_vertex *vb;
    gp89_vertex *vc;
    gp89_fx x1;
    gp89_fx x2;
    gp89_fx y1;
    gp89_fx y2;
    gp89_fx z1;
    gp89_fx z2;
    gp89_fx s1;
    gp89_fx s2;
    gp89_fx t1;
    gp89_fx t2;
    gp89_fx determinant;
    gp89_fx reciprocal;
    gp89_vec3 tangent;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        mesh->vertices[i].tx = 0;
        mesh->vertices[i].ty = 0;
        mesh->vertices[i].tz = 0;
        mesh->vertices[i].tw = GP89_FX_ONE;
    }
    for (i = 0U; i < mesh->triangle_count; ++i) {
        triangle = mesh->triangles[i];
        va = &mesh->vertices[triangle.a];
        vb = &mesh->vertices[triangle.b];
        vc = &mesh->vertices[triangle.c];
        x1 = vb->x - va->x;
        x2 = vc->x - va->x;
        y1 = vb->y - va->y;
        y2 = vc->y - va->y;
        z1 = vb->z - va->z;
        z2 = vc->z - va->z;
        s1 = vb->u - va->u;
        s2 = vc->u - va->u;
        t1 = vb->v - va->v;
        t2 = vc->v - va->v;
        determinant = gp89_fx_mul(s1, t2) - gp89_fx_mul(s2, t1);
        if (gp89_abs_fx(determinant) <= 4) {
            continue;
        }
        reciprocal = gp89_fx_div(GP89_FX_ONE, determinant);
        tangent.x = gp89_fx_mul(gp89_fx_mul(x1, t2) - gp89_fx_mul(x2, t1), reciprocal);
        tangent.y = gp89_fx_mul(gp89_fx_mul(y1, t2) - gp89_fx_mul(y2, t1), reciprocal);
        tangent.z = gp89_fx_mul(gp89_fx_mul(z1, t2) - gp89_fx_mul(z2, t1), reciprocal);
        va->tx += tangent.x;
        va->ty += tangent.y;
        va->tz += tangent.z;
        vb->tx += tangent.x;
        vb->ty += tangent.y;
        vb->tz += tangent.z;
        vc->tx += tangent.x;
        vc->ty += tangent.y;
        vc->tz += tangent.z;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        va = &mesh->vertices[i];
        tangent = gp89_vec3_normalized(gp89_v3(va->tx, va->ty, va->tz));
        va->tx = tangent.x;
        va->ty = tangent.y;
        va->tz = tangent.z;
    }
}

/* ------------------------------------------------------------------------- */
/* Deformation.                                                              */
/* ------------------------------------------------------------------------- */

void gp89_deform_taper_y(gp89_mesh *mesh,
                         gp89_fx bottom_scale,
                         gp89_fx top_scale)
{
    unsigned int i;
    gp89_fx minimum;
    gp89_fx maximum;
    gp89_fx range;
    gp89_fx t;
    gp89_fx scale;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0 || mesh->vertex_count == 0U) {
        return;
    }
    gp89_bounds_axis(mesh, GP89_AXIS_Y, &minimum, &maximum);
    range = maximum - minimum;
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        t = range == 0 ? 0 : gp89_fx_div(vertex->y - minimum, range);
        scale = gp89_fx_lerp(bottom_scale, top_scale, t);
        vertex->x = gp89_fx_mul(vertex->x, scale);
        vertex->z = gp89_fx_mul(vertex->z, scale);
    }
    gp89_recalculate_smooth_normals(mesh);
    gp89_recalculate_tangents(mesh);
}

void gp89_deform_twist_y(gp89_mesh *mesh, gp89_angle total_turn)
{
    unsigned int i;
    gp89_fx minimum;
    gp89_fx maximum;
    gp89_fx range;
    gp89_fx t;
    gp89_angle angle;
    gp89_fx s;
    gp89_fx c;
    gp89_fx x;
    gp89_fx z;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0 || mesh->vertex_count == 0U) {
        return;
    }
    gp89_bounds_axis(mesh, GP89_AXIS_Y, &minimum, &maximum);
    range = maximum - minimum;
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        t = range == 0 ? 0 : gp89_fx_div(vertex->y - minimum, range);
        angle = (gp89_angle)(((gp89_u32)total_turn * (gp89_u32)t) >> 16);
        gp89_sincos(angle, &s, &c);
        x = gp89_fx_mul(vertex->x, c) + gp89_fx_mul(vertex->z, s);
        z = -gp89_fx_mul(vertex->x, s) + gp89_fx_mul(vertex->z, c);
        vertex->x = x;
        vertex->z = z;
    }
    gp89_recalculate_smooth_normals(mesh);
    gp89_recalculate_tangents(mesh);
}

void gp89_deform_shear(gp89_mesh *mesh,
                       gp89_fx x_by_y,
                       gp89_fx z_by_y)
{
    unsigned int i;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        vertex->x += gp89_fx_mul(vertex->y, x_by_y);
        vertex->z += gp89_fx_mul(vertex->y, z_by_y);
    }
    gp89_recalculate_smooth_normals(mesh);
    gp89_recalculate_tangents(mesh);
}

void gp89_deform_spherize(gp89_mesh *mesh,
                          gp89_fx radius,
                          gp89_fx amount)
{
    unsigned int i;
    gp89_vertex *vertex;
    gp89_vec3 direction;
    gp89_vec3 target;

    if (mesh == (gp89_mesh *)0 || radius <= 0) {
        return;
    }
    amount = gp89_clamp_fx(amount, 0, GP89_FX_ONE);
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        direction = gp89_vec3_normalized(gp89_v3(vertex->x, vertex->y, vertex->z));
        target.x = gp89_fx_mul(direction.x, radius);
        target.y = gp89_fx_mul(direction.y, radius);
        target.z = gp89_fx_mul(direction.z, radius);
        vertex->x = gp89_fx_lerp(vertex->x, target.x, amount);
        vertex->y = gp89_fx_lerp(vertex->y, target.y, amount);
        vertex->z = gp89_fx_lerp(vertex->z, target.z, amount);
    }
    gp89_recalculate_smooth_normals(mesh);
    gp89_recalculate_tangents(mesh);
}

void gp89_deform_wave_y(gp89_mesh *mesh,
                        gp89_fx amplitude,
                        gp89_fx x_turns_per_unit,
                        gp89_fx z_turns_per_unit)
{
    unsigned int i;
    gp89_fx phase;
    gp89_fx s;
    gp89_fx c;
    gp89_vertex *vertex;

    if (mesh == (gp89_mesh *)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        vertex = &mesh->vertices[i];
        phase = gp89_fx_mul(vertex->x, x_turns_per_unit) +
                gp89_fx_mul(vertex->z, z_turns_per_unit);
        gp89_sincos((gp89_angle)phase, &s, &c);
        vertex->y += gp89_fx_mul(amplitude, s);
    }
    gp89_recalculate_smooth_normals(mesh);
    gp89_recalculate_tangents(mesh);
}

void gp89_deform_custom(gp89_mesh *mesh,
                        gp89_deform_fn deform,
                        void *user_data,
                        int rebuild_normals_and_tangents)
{
    unsigned int i;

    if (mesh == (gp89_mesh *)0 || deform == (gp89_deform_fn)0) {
        return;
    }
    for (i = 0U; i < mesh->vertex_count; ++i) {
        deform(&mesh->vertices[i], i, user_data);
    }
    if (rebuild_normals_and_tangents) {
        gp89_recalculate_smooth_normals(mesh);
        gp89_recalculate_tangents(mesh);
    }
}
