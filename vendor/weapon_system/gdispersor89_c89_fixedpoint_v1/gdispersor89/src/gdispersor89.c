#include "gdispersor89.h"

static GDP89_Fx gdp89_abs_fx(GDP89_Fx value)
{
    return value < 0L ? -value : value;
}

static GDP89_Fx gdp89_fx_mul(GDP89_Fx a, GDP89_Fx b)
{
    return (a * b) >> GDP89_FIX_SHIFT;
}

static GDP89_Vec3 gdp89_v3_add(GDP89_Vec3 a, GDP89_Vec3 b)
{
    return gdp89_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static GDP89_Vec3 gdp89_v3_scale(GDP89_Vec3 value, GDP89_Fx scale)
{
    return gdp89_v3(gdp89_fx_mul(value.x, scale),
                    gdp89_fx_mul(value.y, scale),
                    gdp89_fx_mul(value.z, scale));
}

static GDP89_Fx gdp89_length_approx(GDP89_Vec3 value)
{
    GDP89_Fx a;
    GDP89_Fx b;
    GDP89_Fx c;
    GDP89_Fx t;

    a = gdp89_abs_fx(value.x);
    b = gdp89_abs_fx(value.y);
    c = gdp89_abs_fx(value.z);

    if (a < b) { t = a; a = b; b = t; }
    if (b < c) { t = b; b = c; c = t; }
    if (a < b) { t = a; a = b; b = t; }

    return a + ((b * 3L) >> 3) + (c >> 2);
}

static void gdp89_zero_request(GDP89_Request *request)
{
    unsigned char *p;
    unsigned long i;
    if (!request) return;
    p = (unsigned char *)request;
    for (i = 0UL; i < (unsigned long)sizeof(*request); i++) p[i] = 0u;
}

static void gdp89_balanced_point(int index, GDP89_Fx *out_x, GDP89_Fx *out_y)
{
    static const short points[32][2] = {
        {    0,    0 },
        {-4096,    0 }, { 4096,    0 },
        {    0,-4096 }, {    0, 4096 },
        {-2896,-2896 }, { 2896,-2896 },
        {-2896, 2896 }, { 2896, 2896 },
        {-2048,    0 }, { 2048,    0 },
        {    0,-2048 }, {    0, 2048 },
        {-1448,-1448 }, { 1448,-1448 },
        {-1448, 1448 }, { 1448, 1448 },
        {-3784,-1567 }, { 3784,-1567 },
        {-3784, 1567 }, { 3784, 1567 },
        {-1567,-3784 }, { 1567,-3784 },
        {-1567, 3784 }, { 1567, 3784 },
        {-3072,    0 }, { 3072,    0 },
        {    0,-3072 }, {    0, 3072 },
        {-1024,-3072 }, { 1024,-3072 },
        {    0, 1024 }
    };
    int p;
    p = index & 31;
    *out_x = (GDP89_Fx)points[p][0];
    *out_y = (GDP89_Fx)points[p][1];
}

static void gdp89_ring_point(int index, GDP89_Fx *out_x, GDP89_Fx *out_y)
{
    static const short ring[16][2] = {
        { 4096,    0 }, { 3784, 1567 }, { 2896, 2896 }, { 1567, 3784 },
        {    0, 4096 }, {-1567, 3784 }, {-2896, 2896 }, {-3784, 1567 },
        {-4096,    0 }, {-3784,-1567 }, {-2896,-2896 }, {-1567,-3784 },
        {    0,-4096 }, { 1567,-3784 }, { 2896,-2896 }, { 3784,-1567 }
    };
    int p;
    p = index & 15;
    *out_x = (GDP89_Fx)ring[p][0];
    *out_y = (GDP89_Fx)ring[p][1];
}

static unsigned long gdp89_hash(unsigned long value)
{
    value ^= value >> 16;
    value *= 1103515245UL;
    value ^= value >> 13;
    value *= 12345UL;
    value ^= value >> 16;
    return value;
}

static void gdp89_hashed_point(int index, int seed, GDP89_Fx *out_x, GDP89_Fx *out_y)
{
    unsigned long h1;
    unsigned long h2;
    long x;
    long y;

    h1 = gdp89_hash((unsigned long)(index + 1) ^ (unsigned long)seed);
    h2 = gdp89_hash(h1 ^ 0x9e3779b9UL);
    x = (long)(h1 & 8191UL) - 4096L;
    y = (long)(h2 & 8191UL) - 4096L;
    *out_x = (GDP89_Fx)x;
    *out_y = (GDP89_Fx)y;
}

static void gdp89_pattern_point(const GDP89_Request *request,
                                int projectile_index,
                                GDP89_Fx *out_x,
                                GDP89_Fx *out_y)
{
    int pattern_index;

    if (request->center_first && projectile_index == 0) {
        *out_x = 0L;
        *out_y = 0L;
        return;
    }

    pattern_index = projectile_index;
    if (request->center_first) pattern_index--;

    if (request->pattern == GDP89_PATTERN_RING) {
        gdp89_ring_point(pattern_index, out_x, out_y);
    } else if (request->pattern == GDP89_PATTERN_HASHED) {
        gdp89_hashed_point(pattern_index, request->shot_seed, out_x, out_y);
    } else {
        gdp89_balanced_point(pattern_index + 1, out_x, out_y);
    }
}

static void gdp89_make_projectile(const GDP89_Request *request,
                                  int index,
                                  int count,
                                  GDP89_Projectile *out_projectile)
{
    GDP89_Fx px;
    GDP89_Fx py;
    GDP89_Fx ox;
    GDP89_Fx oy;
    GDP89_Vec3 direction;
    GDP89_Vec3 right_offset;
    GDP89_Vec3 up_offset;

    out_projectile->flags = 0;
    out_projectile->projectile_index = index;
    out_projectile->projectile_count = count;
    out_projectile->actor_id = request->actor_id;
    out_projectile->actor_kind = request->actor_kind;
    out_projectile->team_id = request->team_id;
    out_projectile->weapon_id = request->weapon_id;
    out_projectile->projectile_id = request->projectile_id;
    out_projectile->user_tag = request->user_tag;
    out_projectile->origin = request->origin;
    out_projectile->speed_fx = request->speed_fx;
    out_projectile->max_distance_fx = request->max_distance_fx;
    out_projectile->life_ms = request->life_ms;

    if (request->damage_mode == GDP89_DAMAGE_SPLIT_TOTAL && count > 0) {
        out_projectile->damage_fx = request->damage_fx / (GDP89_Fx)count;
    } else {
        out_projectile->damage_fx = request->damage_fx;
    }

    if (index == 0) out_projectile->flags |= GDP89_PROJECTILE_FIRST;
    if (index == count - 1) out_projectile->flags |= GDP89_PROJECTILE_LAST;

    px = 0L;
    py = 0L;
    if (request->mode == GDP89_MODE_SPREAD && count > 1 && request->spread_fx != 0L) {
        gdp89_pattern_point(request, index, &px, &py);
    }
    if (px == 0L && py == 0L) out_projectile->flags |= GDP89_PROJECTILE_CENTER;

    ox = gdp89_fx_mul(request->spread_fx, px);
    oy = gdp89_fx_mul(request->spread_fx, py);
    right_offset = gdp89_v3_scale(request->right, ox);
    up_offset = gdp89_v3_scale(request->up, oy);
    direction = gdp89_v3_add(request->forward,
                             gdp89_v3_add(right_offset, up_offset));
    out_projectile->direction = gdp89_v3_normalize(direction);
}

GDP89_Fx gdp89_fx_from_int(int value)
{
    return ((GDP89_Fx)value) << GDP89_FIX_SHIFT;
}

GDP89_Fx gdp89_spread_from_degrees(GDP89_Fx degrees_fx)
{
    /* Small-angle conversion: radians ~= degrees * pi / 180. */
    return (degrees_fx * 71L) >> GDP89_FIX_SHIFT;
}

int gdp89_fx_to_int_round(GDP89_Fx value)
{
    if (value >= 0L) return (int)((value + GDP89_FIX_HALF) >> GDP89_FIX_SHIFT);
    return (int)(-(((-value) + GDP89_FIX_HALF) >> GDP89_FIX_SHIFT));
}

GDP89_Vec3 gdp89_v3(GDP89_Fx x, GDP89_Fx y, GDP89_Fx z)
{
    GDP89_Vec3 value;
    value.x = x;
    value.y = y;
    value.z = z;
    return value;
}

GDP89_Vec3 gdp89_v3_normalize(GDP89_Vec3 value)
{
    GDP89_Fx length;
    GDP89_Vec3 out_value;

    length = gdp89_length_approx(value);
    if (length <= 0L) return gdp89_v3(0L, 0L, GDP89_FIX_ONE);

    out_value.x = (value.x << GDP89_FIX_SHIFT) / length;
    out_value.y = (value.y << GDP89_FIX_SHIFT) / length;
    out_value.z = (value.z << GDP89_FIX_SHIFT) / length;
    return out_value;
}

void gdp89_request_defaults(GDP89_Request *request)
{
    if (!request) return;
    gdp89_zero_request(request);
    request->mode = GDP89_MODE_SINGLE;
    request->pattern = GDP89_PATTERN_BALANCED;
    request->damage_mode = GDP89_DAMAGE_PER_PROJECTILE;
    request->projectile_count = 1;
    request->center_first = 1;
    request->forward = gdp89_v3(0L, 0L, GDP89_FIX_ONE);
    request->right = gdp89_v3(GDP89_FIX_ONE, 0L, 0L);
    request->up = gdp89_v3(0L, GDP89_FIX_ONE, 0L);
}

int gdp89_projectile_count(const GDP89_Request *request)
{
    int count;
    if (!request) return 0;
    if (request->mode != GDP89_MODE_SPREAD) return 1;
    count = request->projectile_count;
    if (count < 1) count = 1;
    if (count > GDP89_MAX_PROJECTILES) count = GDP89_MAX_PROJECTILES;
    return count;
}

int gdp89_build(const GDP89_Request *request,
                GDP89_Projectile *out_projectiles,
                int out_capacity)
{
    int count;
    int i;

    if (!request || !out_projectiles || out_capacity <= 0) return GDP89_BAD_ARG;
    count = gdp89_projectile_count(request);
    if (count > out_capacity) return GDP89_OUTPUT_FULL;

    for (i = 0; i < count; i++) {
        gdp89_make_projectile(request, i, count, &out_projectiles[i]);
    }
    return count;
}

int gdp89_emit(const GDP89_Request *request,
               GDP89_EmitFn emit_fn,
               void *emit_ctx)
{
    GDP89_Projectile projectile;
    int count;
    int i;
    int result;

    if (!request || !emit_fn) return GDP89_BAD_ARG;
    count = gdp89_projectile_count(request);
    for (i = 0; i < count; i++) {
        gdp89_make_projectile(request, i, count, &projectile);
        result = emit_fn(emit_ctx, &projectile);
        if (result != GDP89_OK) return result;
    }
    return count;
}
