#include "gplayerprojectileaim89.h"
#include <string.h>

#define GPPA89_ONE 4096L
#define GPPA89_EPS 1L

static gppa89_fx gppa89_abs(gppa89_fx v) { return v < 0 ? -v : v; }
static gppa89_fx gppa89_max3(gppa89_fx a,gppa89_fx b,gppa89_fx c)
{
    gppa89_fx m;
    m = a > b ? a : b;
    return m > c ? m : c;
}

/* Normalize after first scaling by the largest component. This keeps every
   square inside 32-bit C89 `long` even on MinGW32; integer-only arithmetic. */
static unsigned long gppa89_isqrt(unsigned long value)
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

static gppa89_vec3 gppa89_direction(gppa89_vec3 from,
                                    gppa89_vec3 to,
                                    gppa89_vec3 fallback)
{
    gppa89_vec3 d;
    gppa89_fx m;
    gppa89_fx sx;
    gppa89_fx sy;
    gppa89_fx sz;
    unsigned long sum;
    unsigned long len;
    d.x = to.x - from.x; d.y = to.y - from.y; d.z = to.z - from.z;
    m = gppa89_max3(gppa89_abs(d.x),gppa89_abs(d.y),gppa89_abs(d.z));
    if (m <= GPPA89_EPS) {
        d = fallback;
        m = gppa89_max3(gppa89_abs(d.x),gppa89_abs(d.y),gppa89_abs(d.z));
    }
    if (m <= GPPA89_EPS) { d.x = 0; d.y = 0; d.z = GPPA89_ONE; return d; }
    sx = (d.x * GPPA89_ONE) / m;
    sy = (d.y * GPPA89_ONE) / m;
    sz = (d.z * GPPA89_ONE) / m;
    sum = (unsigned long)(sx * sx) +
          (unsigned long)(sy * sy) +
          (unsigned long)(sz * sz);
    len = gppa89_isqrt(sum);
    if (len == 0UL) { d.x = 0; d.y = 0; d.z = GPPA89_ONE; return d; }
    d.x = (sx * GPPA89_ONE) / (gppa89_fx)len;
    d.y = (sy * GPPA89_ONE) / (gppa89_fx)len;
    d.z = (sz * GPPA89_ONE) / (gppa89_fx)len;
    return d;
}

int gplayerprojectileaim89_resolve(const gppa89_query *query,
                                    gppa89_target_provider_fn target_provider,
                                    void *provider_user,
                                    gppa89_result *result)
{
    gppa89_target target;
    if (!query || !target_provider || !result) return 0;
    memset(result,0,sizeof(*result));
    memset(&target,0,sizeof(target));
    if (!target_provider(provider_user,query,&target) || !target.valid)
        return 0;
    result->valid = 1;
    result->hit = target.hit ? 1 : 0;
    result->blocked_from_muzzle = target.blocked_from_muzzle ? 1 : 0;
    result->target = target.target;
    result->direction = gppa89_direction(query->muzzle_origin,
                                         target.target,
                                         query->fallback_direction);
    return 1;
}
