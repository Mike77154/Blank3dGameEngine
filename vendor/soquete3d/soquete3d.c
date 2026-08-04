#include "soquete3d.h"

#define SOQ3D_S32_MAX ((soq3d_fx)2147483647)
#define SOQ3D_S32_MIN ((soq3d_fx)(-2147483647 - 1))

static unsigned int soq3d_abs_u32(soq3d_fx value)
{
    if (value < 0) {
        return (unsigned int)(-(value + 1)) + 1U;
    }
    return (unsigned int)value;
}

static void soq3d_mul_u32(unsigned int a,
                          unsigned int b,
                          unsigned int *out_hi,
                          unsigned int *out_lo)
{
    unsigned int a0;
    unsigned int a1;
    unsigned int b0;
    unsigned int b1;
    unsigned int p0;
    unsigned int p1;
    unsigned int p2;
    unsigned int p3;
    unsigned int lo;
    unsigned int hi;
    unsigned int old_lo;

    a0 = a & 65535U;
    a1 = a >> 16;
    b0 = b & 65535U;
    b1 = b >> 16;

    p0 = a0 * b0;
    p1 = a0 * b1;
    p2 = a1 * b0;
    p3 = a1 * b1;

    lo = p0;
    hi = p3 + (p1 >> 16);

    old_lo = lo;
    lo += p1 << 16;
    if (lo < old_lo) {
        hi += 1U;
    }

    hi += p2 >> 16;
    old_lo = lo;
    lo += p2 << 16;
    if (lo < old_lo) {
        hi += 1U;
    }

    *out_hi = hi;
    *out_lo = lo;
}

static soq3d_fx soq3d_negate_magnitude_sat(unsigned int magnitude,
                                            int is_negative)
{
    if (!is_negative) {
        if (magnitude > 2147483647U) {
            return SOQ3D_S32_MAX;
        }
        return (soq3d_fx)magnitude;
    }

    if (magnitude >= 2147483648U) {
        return SOQ3D_S32_MIN;
    }
    return (soq3d_fx)(-(soq3d_fx)magnitude);
}

static int soq3d_find_thing(const soq3d_context *ctx, soq3d_key key)
{
    unsigned int i;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_THINGS; ++i) {
        if (ctx->things[i].used && ctx->things[i].key == key) {
            return (int)i;
        }
    }
    return -1;
}

static int soq3d_find_free_thing(const soq3d_context *ctx)
{
    unsigned int i;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_THINGS; ++i) {
        if (!ctx->things[i].used) {
            return (int)i;
        }
    }
    return -1;
}

static int soq3d_find_socket(const soq3d_context *ctx, soq3d_key key)
{
    unsigned int i;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_SOCKETS; ++i) {
        if (ctx->sockets[i].used && ctx->sockets[i].key == key) {
            return (int)i;
        }
    }
    return -1;
}

static int soq3d_find_free_socket(const soq3d_context *ctx)
{
    unsigned int i;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_SOCKETS; ++i) {
        if (!ctx->sockets[i].used) {
            return (int)i;
        }
    }
    return -1;
}

static soq3d_result soq3d_check_stamp(soq3d_stamp actual,
                                      soq3d_stamp required)
{
    if (required != SOQ3D_STAMP_ANY && actual != required) {
        return SOQ3D_ERR_STALE;
    }
    return SOQ3D_OK;
}

static soq3d_fx soq3d_pose_axis(const soq3d_pose *pose, soq3d_axis axis)
{
    if (axis == SOQ3D_AXIS_X) {
        return pose->position.x;
    }
    if (axis == SOQ3D_AXIS_Y) {
        return pose->position.y;
    }
    return pose->position.z;
}

void soq3d_init(soq3d_context *ctx)
{
    unsigned int i;
    if (ctx == 0) {
        return;
    }

    ctx->current_stamp = 0U;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_THINGS; ++i) {
        ctx->things[i].key = SOQ3D_KEY_NONE;
        ctx->things[i].stamp = 0U;
        ctx->things[i].world_pose = soq3d_pose_identity();
        ctx->things[i].used = 0U;
    }
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_SOCKETS; ++i) {
        ctx->sockets[i].key = SOQ3D_KEY_NONE;
        ctx->sockets[i].owner_key = SOQ3D_KEY_NONE;
        ctx->sockets[i].stamp = 0U;
        ctx->sockets[i].pose = soq3d_pose_identity();
        ctx->sockets[i].mode = (unsigned char)SOQ3D_SOCKET_UNUSED;
        ctx->sockets[i].used = 0U;
    }
}

void soq3d_begin_frame(soq3d_context *ctx, soq3d_stamp stamp)
{
    if (ctx != 0) {
        ctx->current_stamp = stamp;
    }
}

soq3d_stamp soq3d_current_stamp(const soq3d_context *ctx)
{
    if (ctx == 0) {
        return 0U;
    }
    return ctx->current_stamp;
}

soq3d_fx soq3d_fx_from_int(int value)
{
    if (value > 32767) {
        return SOQ3D_S32_MAX;
    }
    if (value < -32768) {
        return SOQ3D_S32_MIN;
    }
    return (soq3d_fx)(value * 65536);
}

int soq3d_fx_to_int_floor(soq3d_fx value)
{
    int quotient;
    int remainder;

    quotient = value / 65536;
    remainder = value % 65536;
    if (value < 0 && remainder != 0) {
        quotient -= 1;
    }
    return quotient;
}

soq3d_fx soq3d_fx_add_sat(soq3d_fx a, soq3d_fx b)
{
    if (b > 0 && a > SOQ3D_S32_MAX - b) {
        return SOQ3D_S32_MAX;
    }
    if (b < 0 && a < SOQ3D_S32_MIN - b) {
        return SOQ3D_S32_MIN;
    }
    return (soq3d_fx)(a + b);
}

soq3d_fx soq3d_fx_mul(soq3d_fx a, soq3d_fx b)
{
    unsigned int hi;
    unsigned int lo;
    unsigned int shifted;
    int negative;

    negative = ((a < 0) != (b < 0));
    soq3d_mul_u32(soq3d_abs_u32(a), soq3d_abs_u32(b), &hi, &lo);

    if (hi > 65535U) {
        return negative ? SOQ3D_S32_MIN : SOQ3D_S32_MAX;
    }

    shifted = (hi << 16) | (lo >> 16);
    return soq3d_negate_magnitude_sat(shifted, negative);
}

soq3d_vec3 soq3d_vec3_make(soq3d_fx x, soq3d_fx y, soq3d_fx z)
{
    soq3d_vec3 out;
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

soq3d_basis3 soq3d_basis_identity(void)
{
    soq3d_basis3 out;
    out.m00 = SOQ3D_FX_ONE;
    out.m01 = 0;
    out.m02 = 0;
    out.m10 = 0;
    out.m11 = SOQ3D_FX_ONE;
    out.m12 = 0;
    out.m20 = 0;
    out.m21 = 0;
    out.m22 = SOQ3D_FX_ONE;
    return out;
}

soq3d_pose soq3d_pose_identity(void)
{
    soq3d_pose out;
    out.position = soq3d_vec3_make(0, 0, 0);
    out.basis = soq3d_basis_identity();
    return out;
}

soq3d_pose soq3d_pose_make(soq3d_vec3 position, soq3d_basis3 basis)
{
    soq3d_pose out;
    out.position = position;
    out.basis = basis;
    return out;
}

soq3d_vec3 soq3d_basis_transform_point(const soq3d_basis3 *basis,
                                       const soq3d_vec3 *point)
{
    soq3d_vec3 out;
    soq3d_fx a;
    soq3d_fx b;
    soq3d_fx c;

    if (basis == 0 || point == 0) {
        return soq3d_vec3_make(0, 0, 0);
    }

    a = soq3d_fx_mul(basis->m00, point->x);
    b = soq3d_fx_mul(basis->m01, point->y);
    c = soq3d_fx_mul(basis->m02, point->z);
    out.x = soq3d_fx_add_sat(soq3d_fx_add_sat(a, b), c);

    a = soq3d_fx_mul(basis->m10, point->x);
    b = soq3d_fx_mul(basis->m11, point->y);
    c = soq3d_fx_mul(basis->m12, point->z);
    out.y = soq3d_fx_add_sat(soq3d_fx_add_sat(a, b), c);

    a = soq3d_fx_mul(basis->m20, point->x);
    b = soq3d_fx_mul(basis->m21, point->y);
    c = soq3d_fx_mul(basis->m22, point->z);
    out.z = soq3d_fx_add_sat(soq3d_fx_add_sat(a, b), c);

    return out;
}

soq3d_pose soq3d_pose_compose(const soq3d_pose *parent_world,
                              const soq3d_pose *local_pose)
{
    soq3d_pose out;
    soq3d_vec3 translated;
    const soq3d_basis3 *a;
    const soq3d_basis3 *b;

    if (parent_world == 0 || local_pose == 0) {
        return soq3d_pose_identity();
    }

    translated = soq3d_basis_transform_point(&parent_world->basis,
                                              &local_pose->position);
    out.position.x = soq3d_fx_add_sat(parent_world->position.x, translated.x);
    out.position.y = soq3d_fx_add_sat(parent_world->position.y, translated.y);
    out.position.z = soq3d_fx_add_sat(parent_world->position.z, translated.z);

    a = &parent_world->basis;
    b = &local_pose->basis;

#define SOQ3D_M3_CELL(R, C, A0, A1, A2, B0, B1, B2) \
    out.basis.m##R##C = soq3d_fx_add_sat( \
        soq3d_fx_add_sat(soq3d_fx_mul(a->m##R##A0, b->m##B0##C), \
                         soq3d_fx_mul(a->m##R##A1, b->m##B1##C)), \
        soq3d_fx_mul(a->m##R##A2, b->m##B2##C))

    SOQ3D_M3_CELL(0, 0, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(0, 1, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(0, 2, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(1, 0, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(1, 1, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(1, 2, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(2, 0, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(2, 1, 0, 1, 2, 0, 1, 2);
    SOQ3D_M3_CELL(2, 2, 0, 1, 2, 0, 1, 2);

#undef SOQ3D_M3_CELL

    return out;
}

soq3d_key soq3d_key_from_cstr(const char *text)
{
    soq3d_key hash;
    unsigned char ch;

    if (text == 0 || text[0] == '\0') {
        return SOQ3D_KEY_NONE;
    }

    hash = 2166136261U;
    while (*text != '\0') {
        ch = (unsigned char)*text;
        hash ^= (soq3d_key)ch;
        hash *= 16777619U;
        ++text;
    }

    if (hash == SOQ3D_KEY_NONE) {
        hash = 1U;
    }
    return hash;
}

soq3d_result soq3d_publish_thing(soq3d_context *ctx,
                                 soq3d_key key,
                                 const soq3d_pose *world_pose)
{
    int index;

    if (ctx == 0 || world_pose == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (key == SOQ3D_KEY_NONE) {
        return SOQ3D_ERR_KEY_RESERVED;
    }

    index = soq3d_find_thing(ctx, key);
    if (index < 0) {
        index = soq3d_find_free_thing(ctx);
        if (index < 0) {
            return SOQ3D_ERR_FULL;
        }
    }

    ctx->things[index].key = key;
    ctx->things[index].stamp = ctx->current_stamp;
    ctx->things[index].world_pose = *world_pose;
    ctx->things[index].used = 1U;
    return SOQ3D_OK;
}

soq3d_result soq3d_remove_thing(soq3d_context *ctx, soq3d_key key)
{
    int index;
    if (ctx == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    index = soq3d_find_thing(ctx, key);
    if (index < 0) {
        return SOQ3D_ERR_NOT_FOUND;
    }
    ctx->things[index].used = 0U;
    ctx->things[index].key = SOQ3D_KEY_NONE;
    return SOQ3D_OK;
}

soq3d_result soq3d_get_thing(const soq3d_context *ctx,
                             soq3d_key key,
                             soq3d_stamp required_stamp,
                             soq3d_pose *out_world_pose)
{
    int index;
    soq3d_result freshness;

    if (ctx == 0 || out_world_pose == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (key == SOQ3D_KEY_NONE) {
        return SOQ3D_ERR_KEY_RESERVED;
    }

    index = soq3d_find_thing(ctx, key);
    if (index < 0) {
        return SOQ3D_ERR_NOT_FOUND;
    }

    freshness = soq3d_check_stamp(ctx->things[index].stamp, required_stamp);
    if (freshness != SOQ3D_OK) {
        return freshness;
    }

    *out_world_pose = ctx->things[index].world_pose;
    return SOQ3D_OK;
}

soq3d_result soq3d_define_local_socket(soq3d_context *ctx,
                                       soq3d_key socket_key,
                                       soq3d_key owner_thing_key,
                                       const soq3d_pose *local_pose)
{
    int index;

    if (ctx == 0 || local_pose == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (socket_key == SOQ3D_KEY_NONE || owner_thing_key == SOQ3D_KEY_NONE) {
        return SOQ3D_ERR_KEY_RESERVED;
    }

    index = soq3d_find_socket(ctx, socket_key);
    if (index < 0) {
        index = soq3d_find_free_socket(ctx);
        if (index < 0) {
            return SOQ3D_ERR_FULL;
        }
    }

    ctx->sockets[index].key = socket_key;
    ctx->sockets[index].owner_key = owner_thing_key;
    ctx->sockets[index].stamp = 0U;
    ctx->sockets[index].pose = *local_pose;
    ctx->sockets[index].mode = (unsigned char)SOQ3D_SOCKET_LOCAL_TO_THING;
    ctx->sockets[index].used = 1U;
    return SOQ3D_OK;
}

soq3d_result soq3d_publish_world_socket(soq3d_context *ctx,
                                        soq3d_key socket_key,
                                        const soq3d_pose *world_pose)
{
    int index;

    if (ctx == 0 || world_pose == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (socket_key == SOQ3D_KEY_NONE) {
        return SOQ3D_ERR_KEY_RESERVED;
    }

    index = soq3d_find_socket(ctx, socket_key);
    if (index < 0) {
        index = soq3d_find_free_socket(ctx);
        if (index < 0) {
            return SOQ3D_ERR_FULL;
        }
    }

    ctx->sockets[index].key = socket_key;
    ctx->sockets[index].owner_key = SOQ3D_KEY_NONE;
    ctx->sockets[index].stamp = ctx->current_stamp;
    ctx->sockets[index].pose = *world_pose;
    ctx->sockets[index].mode = (unsigned char)SOQ3D_SOCKET_WORLD_PUBLISHED;
    ctx->sockets[index].used = 1U;
    return SOQ3D_OK;
}

soq3d_result soq3d_remove_socket(soq3d_context *ctx, soq3d_key socket_key)
{
    int index;
    if (ctx == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    index = soq3d_find_socket(ctx, socket_key);
    if (index < 0) {
        return SOQ3D_ERR_NOT_FOUND;
    }
    ctx->sockets[index].used = 0U;
    ctx->sockets[index].key = SOQ3D_KEY_NONE;
    ctx->sockets[index].owner_key = SOQ3D_KEY_NONE;
    ctx->sockets[index].mode = (unsigned char)SOQ3D_SOCKET_UNUSED;
    return SOQ3D_OK;
}

soq3d_result soq3d_get_socket(const soq3d_context *ctx,
                              soq3d_key socket_key,
                              soq3d_stamp required_stamp,
                              soq3d_pose *out_world_pose)
{
    int index;
    soq3d_result result;
    soq3d_pose owner_pose;

    if (ctx == 0 || out_world_pose == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (socket_key == SOQ3D_KEY_NONE) {
        return SOQ3D_ERR_KEY_RESERVED;
    }

    index = soq3d_find_socket(ctx, socket_key);
    if (index < 0) {
        return SOQ3D_ERR_NOT_FOUND;
    }

    if (ctx->sockets[index].mode == (unsigned char)SOQ3D_SOCKET_WORLD_PUBLISHED) {
        result = soq3d_check_stamp(ctx->sockets[index].stamp, required_stamp);
        if (result != SOQ3D_OK) {
            return result;
        }
        *out_world_pose = ctx->sockets[index].pose;
        return SOQ3D_OK;
    }

    if (ctx->sockets[index].mode == (unsigned char)SOQ3D_SOCKET_LOCAL_TO_THING) {
        result = soq3d_get_thing(ctx,
                                 ctx->sockets[index].owner_key,
                                 required_stamp,
                                 &owner_pose);
        if (result != SOQ3D_OK) {
            return result;
        }
        *out_world_pose = soq3d_pose_compose(&owner_pose,
                                             &ctx->sockets[index].pose);
        return SOQ3D_OK;
    }

    return SOQ3D_ERR_NOT_FOUND;
}

soq3d_result soq3d_get_thing_height(const soq3d_context *ctx,
                                    soq3d_key key,
                                    soq3d_stamp required_stamp,
                                    soq3d_axis up_axis,
                                    soq3d_fx *out_height)
{
    soq3d_pose pose;
    soq3d_result result;

    if (out_height == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (up_axis != SOQ3D_AXIS_X &&
        up_axis != SOQ3D_AXIS_Y &&
        up_axis != SOQ3D_AXIS_Z) {
        return SOQ3D_ERR_BAD_AXIS;
    }

    result = soq3d_get_thing(ctx, key, required_stamp, &pose);
    if (result != SOQ3D_OK) {
        return result;
    }
    *out_height = soq3d_pose_axis(&pose, up_axis);
    return SOQ3D_OK;
}

soq3d_result soq3d_get_socket_height(const soq3d_context *ctx,
                                     soq3d_key key,
                                     soq3d_stamp required_stamp,
                                     soq3d_axis up_axis,
                                     soq3d_fx *out_height)
{
    soq3d_pose pose;
    soq3d_result result;

    if (out_height == 0) {
        return SOQ3D_ERR_BAD_ARG;
    }
    if (up_axis != SOQ3D_AXIS_X &&
        up_axis != SOQ3D_AXIS_Y &&
        up_axis != SOQ3D_AXIS_Z) {
        return SOQ3D_ERR_BAD_AXIS;
    }

    result = soq3d_get_socket(ctx, key, required_stamp, &pose);
    if (result != SOQ3D_OK) {
        return result;
    }
    *out_height = soq3d_pose_axis(&pose, up_axis);
    return SOQ3D_OK;
}

unsigned int soq3d_count_things(const soq3d_context *ctx)
{
    unsigned int i;
    unsigned int count;
    if (ctx == 0) {
        return 0U;
    }
    count = 0U;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_THINGS; ++i) {
        if (ctx->things[i].used) {
            ++count;
        }
    }
    return count;
}

unsigned int soq3d_count_sockets(const soq3d_context *ctx)
{
    unsigned int i;
    unsigned int count;
    if (ctx == 0) {
        return 0U;
    }
    count = 0U;
    for (i = 0U; i < (unsigned int)SOQ3D_MAX_SOCKETS; ++i) {
        if (ctx->sockets[i].used) {
            ++count;
        }
    }
    return count;
}

const char *soq3d_result_string(soq3d_result result)
{
    switch (result) {
        case SOQ3D_OK: return "ok";
        case SOQ3D_ERR_BAD_ARG: return "bad argument";
        case SOQ3D_ERR_KEY_RESERVED: return "key zero is reserved";
        case SOQ3D_ERR_FULL: return "static capacity exhausted";
        case SOQ3D_ERR_NOT_FOUND: return "not found";
        case SOQ3D_ERR_STALE: return "pose is stale for requested stamp";
        case SOQ3D_ERR_BAD_AXIS: return "invalid axis";
        default: return "unknown result";
    }
}
