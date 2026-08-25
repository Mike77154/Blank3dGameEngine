#include "bulletinline89.h"

#include <string.h>

static unsigned long bi89_isqrt(unsigned long value)
{
    unsigned long op;
    unsigned long res;
    unsigned long one;
    op = value;
    res = 0UL;
    one = 1UL << 30;
    while (one > op) one >>= 2;
    while (one != 0UL) {
        if (op >= res + one) {
            op -= res + one;
            res = res + (one << 1);
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

static bi89_fx bi89_abs(bi89_fx v) { return v < 0L ? -v : v; }

static int bi89_normalize(bi89_vec3 value, bi89_vec3 *out)
{
    bi89_fx maxc;
    bi89_fx sx;
    bi89_fx sy;
    bi89_fx sz;
    unsigned long sum;
    unsigned long len;
    if (!out) return 0;
    maxc = bi89_abs(value.x);
    if (bi89_abs(value.y) > maxc) maxc = bi89_abs(value.y);
    if (bi89_abs(value.z) > maxc) maxc = bi89_abs(value.z);
    if (maxc <= 0L) return 0;
    sx = (value.x * BI89_ONE) / maxc;
    sy = (value.y * BI89_ONE) / maxc;
    sz = (value.z * BI89_ONE) / maxc;
    sum = (unsigned long)(sx * sx) +
          (unsigned long)(sy * sy) +
          (unsigned long)(sz * sz);
    len = bi89_isqrt(sum);
    if (len == 0UL) return 0;
    out->x = (sx * BI89_ONE) / (bi89_fx)len;
    out->y = (sy * BI89_ONE) / (bi89_fx)len;
    out->z = (sz * BI89_ONE) / (bi89_fx)len;
    return 1;
}

int bulletinline89_resolve(const bi89_request *request,
                           bi89_result *result)
{
    bi89_vec3 delta;
    if (!request || !result) return 0;
    memset(result, 0, sizeof(*result));
    if (request->target_valid) {
        delta.x = request->target.x - request->origin.x;
        delta.y = request->target.y - request->origin.y;
        delta.z = request->target.z - request->origin.z;
        if (bi89_normalize(delta, &result->direction)) {
            result->valid = 1;
            result->used_target = 1;
            return 1;
        }
    }
    if (!bi89_normalize(request->fallback_direction, &result->direction))
        return 0;
    result->valid = 1;
    result->used_target = 0;
    return 1;
}
