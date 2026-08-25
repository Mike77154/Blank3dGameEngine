#include "bulletcircle89.h"

#include <string.h>

/* atan(2^-i) expressed as one full turn == 65536. */
static const long bc89_atan_turn_q16[15] = {
    8192L, 4836L, 2555L, 1297L, 651L,
    326L, 163L, 81L, 41L, 20L,
    10L, 5L, 3L, 1L, 1L
};

static long bc89_wrap_turn(long turn)
{
    while (turn < 0L) turn += BC89_TURN_ONE;
    while (turn >= BC89_TURN_ONE) turn -= BC89_TURN_ONE;
    return turn;
}

static void bc89_sincos(long turn_q16, long *sin_q12, long *cos_q12)
{
    long z;
    long x;
    long y;
    long nx;
    long ny;
    long shift;
    int i;
    int flip;
    z = bc89_wrap_turn(turn_q16);
    if (z >= 32768L) z -= 65536L;
    flip = 0;
    if (z > 16384L) {
        z -= 32768L;
        flip = 1;
    } else if (z < -16384L) {
        z += 32768L;
        flip = 1;
    }
    /* CORDIC gain compensation in Q12: 0.607252935 * 4096. */
    x = 2487L;
    y = 0L;
    for (i = 0; i < 15; ++i) {
        shift = 1L << i;
        if (z >= 0L) {
            nx = x - (y / shift);
            ny = y + (x / shift);
            z -= bc89_atan_turn_q16[i];
        } else {
            nx = x + (y / shift);
            ny = y - (x / shift);
            z += bc89_atan_turn_q16[i];
        }
        x = nx;
        y = ny;
    }
    if (flip) {
        x = -x;
        y = -y;
    }
    if (sin_q12) *sin_q12 = y;
    if (cos_q12) *cos_q12 = x;
}

static bc89_fx bc89_mul_q12(bc89_fx a, bc89_fx b)
{
    return (bc89_fx)((a * b) / BC89_ONE);
}

int bulletcircle89_resolve(const bc89_request *request,
                           bc89_result *result)
{
    long angle;
    long s;
    long c;
    bc89_fx radial_right;
    bc89_fx radial_up;
    int slot;
    if (!request || !result) return 0;
    memset(result, 0, sizeof(*result));
    if (request->slot_count < 1 || request->radius_fx < 0L) return 0;
    slot = request->slot_index;
    while (slot < 0) slot += request->slot_count;
    while (slot >= request->slot_count) slot -= request->slot_count;
    angle = request->phase_turn_q16 +
            ((long)slot * BC89_TURN_ONE) / (long)request->slot_count;
    angle = bc89_wrap_turn(angle);
    bc89_sincos(angle, &s, &c);
    radial_right = bc89_mul_q12(request->radius_fx, (bc89_fx)c);
    radial_up = bc89_mul_q12(request->radius_fx, (bc89_fx)s);
    result->offset.x = bc89_mul_q12(request->right.x, radial_right) +
                       bc89_mul_q12(request->up.x, radial_up);
    result->offset.y = bc89_mul_q12(request->right.y, radial_right) +
                       bc89_mul_q12(request->up.y, radial_up);
    result->offset.z = bc89_mul_q12(request->right.z, radial_right) +
                       bc89_mul_q12(request->up.z, radial_up);
    result->origin.x = request->center.x + result->offset.x;
    result->origin.y = request->center.y + result->offset.y;
    result->origin.z = request->center.z + result->offset.z;
    result->slot_index = slot;
    result->angle_turn_q16 = angle;
    result->valid = 1;
    return 1;
}
