#include "telesearcher89.h"
#include <string.h>

static ts89_fx ts89_abs(ts89_fx v) { return v < 0 ? -v : v; }
static ts89_fx ts89_max3(ts89_fx a, ts89_fx b, ts89_fx c)
{
    ts89_fx m;
    m = a > b ? a : b;
    return m > c ? m : c;
}

static unsigned long ts89_isqrt(unsigned long value)
{
    unsigned long result;
    unsigned long bit;
    result = 0UL;
    bit = 1UL << 30;
    while (bit > value) bit >>= 2;
    while (bit != 0UL) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

static ts89_vec3 ts89_unit_to(ts89_vec3 from, ts89_vec3 to,
                              ts89_vec3 fallback)
{
    ts89_vec3 d;
    ts89_fx m;
    ts89_fx sx;
    ts89_fx sy;
    ts89_fx sz;
    unsigned long sum;
    unsigned long len;
    d.x = to.x - from.x;
    d.y = to.y - from.y;
    d.z = to.z - from.z;
    m = ts89_max3(ts89_abs(d.x), ts89_abs(d.y), ts89_abs(d.z));
    if (m <= 1L) {
        d = fallback;
        m = ts89_max3(ts89_abs(d.x), ts89_abs(d.y), ts89_abs(d.z));
    }
    if (m <= 1L) {
        d.x = 0L; d.y = 0L; d.z = TS89_ONE;
        return d;
    }
    sx = (d.x * TS89_ONE) / m;
    sy = (d.y * TS89_ONE) / m;
    sz = (d.z * TS89_ONE) / m;
    sum = (unsigned long)(sx * sx) +
          (unsigned long)(sy * sy) +
          (unsigned long)(sz * sz);
    len = ts89_isqrt(sum);
    if (len == 0UL) {
        d.x = 0L; d.y = 0L; d.z = TS89_ONE;
        return d;
    }
    d.x = (sx * TS89_ONE) / (ts89_fx)len;
    d.y = (sy * TS89_ONE) / (ts89_fx)len;
    d.z = (sz * TS89_ONE) / (ts89_fx)len;
    return d;
}

static ts89_fx ts89_speed(ts89_vec3 velocity)
{
    ts89_vec3 zero;
    ts89_vec3 unit;
    ts89_fx m;
    zero.x = zero.y = zero.z = 0L;
    m = ts89_max3(ts89_abs(velocity.x), ts89_abs(velocity.y),
                  ts89_abs(velocity.z));
    if (m <= 1L) return TS89_ONE;
    unit = ts89_unit_to(zero, velocity, velocity);
    if (ts89_abs(unit.x) > 0L)
        return (velocity.x * TS89_ONE) / unit.x;
    if (ts89_abs(unit.y) > 0L)
        return (velocity.y * TS89_ONE) / unit.y;
    if (ts89_abs(unit.z) > 0L)
        return (velocity.z * TS89_ONE) / unit.z;
    return m;
}

static ts89_fx ts89_mix(ts89_fx a, ts89_fx b, ts89_fx gain)
{
    ts89_fx delta;
    if (gain <= 0L) return a;
    if (gain >= TS89_ONE) return b;
    delta = b - a;
    return a + (delta * gain) / TS89_ONE;
}

int telesearcher89_step(const ts89_request *request,
                         ts89_target_provider_fn target_provider,
                         void *provider_user,
                         ts89_result *result)
{
    ts89_target target;
    ts89_vec3 target_point;
    ts89_vec3 direction;
    ts89_vec3 desired;
    ts89_fx speed;
    ts89_fx gain;
    int requested;
    if (!request || !result) return 0;
    memset(result, 0, sizeof(*result));
    result->target_actor_id = request->target_actor_id;
    result->velocity = request->velocity;
    if (!target_provider) {
        if (request->flags & TS89_FLAG_REQUIRE_TARGET) return 0;
        result->valid = 1;
        return 1;
    }
    requested = request->target_actor_id;
    if ((request->flags & TS89_FLAG_REACQUIRE) != 0UL)
        requested = TS89_TARGET_NONE;
    memset(&target, 0, sizeof(target));
    target.actor_id = TS89_TARGET_NONE;
    if (!target_provider(provider_user, request->owner_actor_id,
                         requested, &target) || !target.valid) {
        if (request->flags & TS89_FLAG_REQUIRE_TARGET) return 0;
        result->valid = 1;
        return 1;
    }
    target_point = target.position;
    target_point.x += request->target_offset.x;
    target_point.y += request->target_offset.y;
    target_point.z += request->target_offset.z;
    speed = request->speed_fx > 0L ? request->speed_fx
                                  : ts89_speed(request->velocity);
    direction = ts89_unit_to(request->position, target_point,
                             request->velocity);
    desired.x = (direction.x * speed) / TS89_ONE;
    desired.y = (direction.y * speed) / TS89_ONE;
    desired.z = (direction.z * speed) / TS89_ONE;
    gain = request->gain_fx;
    if (gain <= 0L) gain = TS89_ONE;
    if (gain > TS89_ONE) gain = TS89_ONE;
    result->valid = 1;
    result->target_found = 1;
    result->target_actor_id = target.actor_id;
    result->target_position = target_point;
    result->velocity.x = ts89_mix(request->velocity.x, desired.x, gain);
    result->velocity.y = ts89_mix(request->velocity.y, desired.y, gain);
    result->velocity.z = ts89_mix(request->velocity.z, desired.z, gain);
    return 1;
}
