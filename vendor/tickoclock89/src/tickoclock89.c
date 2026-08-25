#include "tickoclock89.h"
#include <string.h>

static toc89_u32 toc89_sat_add(toc89_u32 a, toc89_u32 b)
{
    if (a > UINT_MAX - b) return UINT_MAX;
    return a + b;
}

static toc89_u32 toc89_sat_mul(toc89_u32 a, toc89_u32 b)
{
    if (a == 0U || b == 0U) return 0U;
    if (a > UINT_MAX / b) return UINT_MAX;
    return a * b;
}

static int toc89_crossed(toc89_u32 previous, toc89_u32 current,
                         toc89_u32 period)
{
    if (period == 0U || current < previous) return 0;
    return previous / period != current / period;
}

static void toc89_advance_hms(TickOClock89HMS *hms, toc89_u32 delta_ms)
{
    toc89_u32 carry;
    toc89_u32 value;

    if (!hms || delta_ms == 0U) return;

    value = (toc89_u32)hms->milliseconds + delta_ms;
    hms->milliseconds = (toc89_i32)(value % 1000U);
    carry = value / 1000U;

    value = (toc89_u32)hms->seconds + carry;
    hms->seconds = (toc89_i32)(value % 60U);
    carry = value / 60U;

    value = (toc89_u32)hms->minutes + carry;
    hms->minutes = (toc89_i32)(value % 60U);
    carry = value / 60U;

    value = (toc89_u32)hms->hours + carry;
    hms->hours = (toc89_i32)(value % 24U);
    carry = value / 24U;
    hms->days = toc89_sat_add(hms->days, carry);
}

void tickoclock89_init(TickOClock89 *clock_value, toc89_u32 tick_rate)
{
    if (!clock_value) return;
    memset(clock_value, 0, sizeof(*clock_value));
    clock_value->tick_rate = tick_rate ? tick_rate : 60U;
}

void tickoclock89_reset(TickOClock89 *clock_value)
{
    toc89_u32 rate;
    if (!clock_value) return;
    rate = clock_value->tick_rate;
    tickoclock89_init(clock_value, rate);
}

void tickoclock89_set_tick_rate(TickOClock89 *clock_value,
                                toc89_u32 tick_rate)
{
    if (!clock_value) return;
    clock_value->tick_rate = tick_rate ? tick_rate : 60U;
    clock_value->tick_remainder_milli = 0U;
}

void tickoclock89_set_hms(TickOClock89 *clock_value,
                          toc89_i32 hours,
                          toc89_i32 minutes,
                          toc89_i32 seconds,
                          toc89_i32 milliseconds)
{
    if (!clock_value) return;
    if (hours < 0) hours = 0;
    if (minutes < 0) minutes = 0;
    if (seconds < 0) seconds = 0;
    if (milliseconds < 0) milliseconds = 0;
    clock_value->hms.hours = hours % 24;
    clock_value->hms.minutes = minutes % 60;
    clock_value->hms.seconds = seconds % 60;
    clock_value->hms.milliseconds = milliseconds % 1000;
}

void tickoclock89_update(TickOClock89 *clock_value,
                         toc89_u32 delta_ms,
                         toc89_u32 delta_frames)
{
    toc89_u32 scaled;
    toc89_u32 accumulator;
    toc89_u32 ticks;

    if (!clock_value) return;

    clock_value->previous_ticks = clock_value->global_ticks;
    clock_value->previous_ms = clock_value->global_ms;
    clock_value->previous_frames = clock_value->global_frames;
    clock_value->delta_ms = delta_ms;
    clock_value->delta_frames = delta_frames;

    clock_value->global_ms =
        toc89_sat_add(clock_value->global_ms, delta_ms);
    clock_value->global_frames =
        toc89_sat_add(clock_value->global_frames, delta_frames);

    scaled = toc89_sat_mul(delta_ms, clock_value->tick_rate);
    accumulator = toc89_sat_add(clock_value->tick_remainder_milli, scaled);
    ticks = accumulator / 1000U;
    clock_value->tick_remainder_milli = accumulator % 1000U;
    clock_value->delta_ticks = ticks;
    clock_value->global_ticks =
        toc89_sat_add(clock_value->global_ticks, ticks);

    toc89_advance_hms(&clock_value->hms, delta_ms);
}

toc89_u32 tickoclock89_delta_ticks(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->delta_ticks : 0U;
}
toc89_u32 tickoclock89_delta_ms(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->delta_ms : 0U;
}
toc89_u32 tickoclock89_delta_frames(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->delta_frames : 0U;
}
toc89_fixed tickoclock89_frametime_q16(const TickOClock89 *clock_value)
{
    toc89_u32 ms;
    toc89_u32 q;
    if (!clock_value) return 0;
    ms = clock_value->delta_ms;
    if (ms >= 32767000U) return INT_MAX;
    q = (ms / 1000U) * 65536U;
    q += ((ms % 1000U) * 65536U) / 1000U;
    if (q > (toc89_u32)INT_MAX) return INT_MAX;
    return (toc89_fixed)q;
}
toc89_u32 tickoclock89_global_ticks(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->global_ticks : 0U;
}
toc89_u32 tickoclock89_global_ms(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->global_ms : 0U;
}
toc89_u32 tickoclock89_global_frames(const TickOClock89 *clock_value)
{
    return clock_value ? clock_value->global_frames : 0U;
}
void tickoclock89_get_hms(const TickOClock89 *clock_value,
                          TickOClock89HMS *out_hms)
{
    if (!out_hms) return;
    if (!clock_value) {
        memset(out_hms, 0, sizeof(*out_hms));
        return;
    }
    *out_hms = clock_value->hms;
}

int tickoclock89_every_ticks(const TickOClock89 *clock_value,
                             toc89_u32 period)
{
    if (!clock_value) return 0;
    return toc89_crossed(clock_value->previous_ticks,
                         clock_value->global_ticks, period);
}
int tickoclock89_every_ms(const TickOClock89 *clock_value,
                          toc89_u32 period)
{
    if (!clock_value) return 0;
    return toc89_crossed(clock_value->previous_ms,
                         clock_value->global_ms, period);
}
int tickoclock89_every_frames(const TickOClock89 *clock_value,
                              toc89_u32 period)
{
    if (!clock_value) return 0;
    return toc89_crossed(clock_value->previous_frames,
                         clock_value->global_frames, period);
}
int tickoclock89_every_seconds(const TickOClock89 *clock_value,
                               toc89_u32 period)
{
    if (period == 0U || period > UINT_MAX / 1000U) return 0;
    return tickoclock89_every_ms(clock_value, period * 1000U);
}
int tickoclock89_every_minutes(const TickOClock89 *clock_value,
                               toc89_u32 period)
{
    if (period == 0U || period > UINT_MAX / 60000U) return 0;
    return tickoclock89_every_ms(clock_value, period * 60000U);
}

static int toc89_name_eq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    return strcmp(a, b) == 0;
}

int tickoclock89_unit_from_name(const char *name, int *out_unit)
{
    int unit;
    if (!name || !out_unit) return 0;
    unit = 0;
    if (toc89_name_eq(name, "tick") || toc89_name_eq(name, "ticks"))
        unit = TOC89_UNIT_TICKS;
    else if (toc89_name_eq(name, "ms") ||
             toc89_name_eq(name, "millisecond") ||
             toc89_name_eq(name, "milliseconds"))
        unit = TOC89_UNIT_MILLISECONDS;
    else if (toc89_name_eq(name, "frame") ||
             toc89_name_eq(name, "frames") ||
             toc89_name_eq(name, "frametime"))
        unit = TOC89_UNIT_FRAMES;
    else if (toc89_name_eq(name, "second") ||
             toc89_name_eq(name, "seconds") ||
             toc89_name_eq(name, "sec"))
        unit = TOC89_UNIT_SECONDS;
    else if (toc89_name_eq(name, "minute") ||
             toc89_name_eq(name, "minutes") ||
             toc89_name_eq(name, "min"))
        unit = TOC89_UNIT_MINUTES;
    else if (toc89_name_eq(name, "hour") ||
             toc89_name_eq(name, "hours") ||
             toc89_name_eq(name, "hr"))
        unit = TOC89_UNIT_HOURS;
    if (!unit) return 0;
    *out_unit = unit;
    return 1;
}

toc89_u32 tickoclock89_value(const TickOClock89 *clock_value, int unit)
{
    if (!clock_value) return 0U;
    switch (unit) {
        case TOC89_UNIT_TICKS: return clock_value->global_ticks;
        case TOC89_UNIT_MILLISECONDS: return clock_value->global_ms;
        case TOC89_UNIT_FRAMES: return clock_value->global_frames;
        case TOC89_UNIT_SECONDS: return clock_value->global_ms / 1000U;
        case TOC89_UNIT_MINUTES: return clock_value->global_ms / 60000U;
        case TOC89_UNIT_HOURS: return clock_value->global_ms / 3600000U;
        default: return 0U;
    }
}
