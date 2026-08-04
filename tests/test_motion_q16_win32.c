/*
 * test_motion_q16_win32.c
 * Regression for the 32-bit signed-long Q16.16 overflow that erased X/Z
 * attack direction on MinGW32.  This test intentionally uses signed int as
 * the vendor scalar so the same width is exercised on 64-bit build hosts.
 */
#define GAL_FX_TYPE signed int
#define GGL_FX_TYPE signed int

#include "gairlunge89.h"
#include "ggroundlance89.h"

#include <limits.h>
#include <stdio.h>

static int test_air_math(void)
{
    gal_fx divided;
    gal_fx multiplied;
    gal_vec3 source;
    gal_vec3 direction;

    divided = gal_fx_div(GAL_FX_FROM_INT(14), GAL_FX_FROM_INT(39));
    if (divided <= 0) return 1;

    multiplied = gal_fx_mul(GAL_FX_FROM_INT(14), GAL_FX_HALF);
    if (multiplied != GAL_FX_FROM_INT(7)) return 2;

    source.x = GAL_FX_FROM_INT(14);
    source.y = (GAL_FX_ONE * 9) / 10;
    source.z = -GAL_FX_FROM_INT(34);
    gal_vec3_normalize_approx(&source, &direction);
    if (direction.x <= 0) return 3;
    if (direction.z >= 0) return 4;
    if (direction.x == 0 || direction.z == 0) return 5;
    return 0;
}

static int test_ground_math(void)
{
    ggl_fx divided;
    ggl_fx multiplied;
    ggl_vec3 source;
    ggl_vec3 direction;

    divided = ggl_fx_div(GGL_FX_FROM_INT(14), GGL_FX_FROM_INT(39));
    if (divided <= 0) return 1;

    multiplied = ggl_fx_mul(GGL_FX_FROM_INT(14), GGL_FX_HALF);
    if (multiplied != GGL_FX_FROM_INT(7)) return 2;

    source.x = -GGL_FX_FROM_INT(14);
    source.y = 0;
    source.z = GGL_FX_FROM_INT(34);
    ggl_vec3_normalize_approx(&source, &direction);
    if (direction.x >= 0) return 3;
    if (direction.z <= 0) return 4;
    if (direction.x == 0 || direction.z == 0) return 5;
    return 0;
}

int main(void)
{
    int result;

    if (INT_MAX < 2147483647) {
        puts("motion Q16 regression requires at least 32-bit signed int");
        return 90;
    }

    result = test_air_math();
    if (result != 0) {
        printf("gairlunge89 32-bit Q16 regression failed: %d\n", result);
        return result;
    }

    result = test_ground_math();
    if (result != 0) {
        printf("ggroundlance89 32-bit Q16 regression failed: %d\n", result);
        return 20 + result;
    }

    puts("Motion89 32-bit Q16 multiply/divide regression: OK");
    return 0;
}
