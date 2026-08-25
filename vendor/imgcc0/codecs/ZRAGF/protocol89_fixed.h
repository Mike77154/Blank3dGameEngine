#ifndef ZRAGF_PROTOCOL89_FIXED_H_INCLUDED
#define ZRAGF_PROTOCOL89_FIXED_H_INCLUDED

#include <time.h>
#include "zragflib.h"

/* Fixed decimal scalar. Public benchmark ratios and rates use x1000.
   Elapsed time helpers return integer milliseconds. */
typedef int zragf_fx;

static zragf_fx zragf_fx_ratio_1000(zragf_size_t num, zragf_size_t den)
{
    zragf_size_t whole;
    zragf_size_t rem;
    zragf_size_t digit;
    zragf_size_t scale;
    zragf_size_t out;
    int i;
    if (den == 0u)
        return 0;
    whole = num / den;
    rem = num % den;
    if (whole > 2147483u)
        return 2147483647;
    out = whole * 1000u;
    scale = 100u;
    for (i = 0; i < 3; ++i) {
        rem *= 10u;
        digit = rem / den;
        rem %= den;
        out += digit * scale;
        scale /= 10u;
    }
    return (zragf_fx)out;
}

static zragf_fx zragf_fx_millis(clock_t ticks)
{
    clock_t whole;
    clock_t rem;
    zragf_fx ms;
    if (ticks <= (clock_t)0)
        return 0;
    whole = ticks / (clock_t)CLOCKS_PER_SEC;
    rem = ticks % (clock_t)CLOCKS_PER_SEC;
    if (whole > (clock_t)2147483)
        return 2147483647;
    ms = (zragf_fx)whole * 1000;
    ms += (zragf_fx)((rem * (clock_t)1000) / (clock_t)CLOCKS_PER_SEC);
    if (ms == 0)
        ms = 1;
    return ms;
}

static zragf_fx zragf_fx_avg_nonzero(zragf_fx value, int count)
{
    zragf_fx out;
    if (count <= 0)
        return 0;
    out = value / count;
    if (value > 0 && out == 0)
        out = 1;
    return out;
}

static zragf_fx zragf_fx_mibs_1000(zragf_size_t bytes, zragf_fx millis)
{
    zragf_fx mib_x1000;
    if (millis <= 0)
        return 0;
    mib_x1000 = zragf_fx_ratio_1000(bytes, (zragf_size_t)1048576u);
    if (mib_x1000 > 2147483)
        return 2147483647;
    return (mib_x1000 * 1000) / millis;
}

#endif
