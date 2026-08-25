#ifndef TICKOCLOCK89_H
#define TICKOCLOCK89_H

/*
 * TickOClock89
 * Multi-view simulation clock: ticks, milliseconds, frames and HMS.
 * C89, fixed storage, integer/fixed-point only, no OS ownership.
 */

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if INT_MAX < 2147483647
#error TickOClock89 requires an at-least-32-bit signed int
#endif
#if UINT_MAX < 4294967295U
#error TickOClock89 requires an at-least-32-bit unsigned int
#endif

typedef signed int toc89_i32;
typedef unsigned int toc89_u32;
typedef toc89_i32 toc89_fixed;

#define TOC89_FP_ONE ((toc89_fixed)65536)

typedef enum TickOClock89Unit {
    TOC89_UNIT_TICKS = 1,
    TOC89_UNIT_MILLISECONDS = 2,
    TOC89_UNIT_FRAMES = 3,
    TOC89_UNIT_SECONDS = 4,
    TOC89_UNIT_MINUTES = 5,
    TOC89_UNIT_HOURS = 6
} TickOClock89Unit;

typedef struct TickOClock89HMS {
    toc89_u32 days;
    toc89_i32 hours;
    toc89_i32 minutes;
    toc89_i32 seconds;
    toc89_i32 milliseconds;
} TickOClock89HMS;

typedef struct TickOClock89 {
    toc89_u32 tick_rate;
    toc89_u32 tick_remainder_milli;

    toc89_u32 previous_ticks;
    toc89_u32 global_ticks;
    toc89_u32 delta_ticks;

    toc89_u32 previous_ms;
    toc89_u32 global_ms;
    toc89_u32 delta_ms;

    toc89_u32 previous_frames;
    toc89_u32 global_frames;
    toc89_u32 delta_frames;

    TickOClock89HMS hms;
} TickOClock89;

void tickoclock89_init(TickOClock89 *clock_value, toc89_u32 tick_rate);
void tickoclock89_reset(TickOClock89 *clock_value);
void tickoclock89_set_tick_rate(TickOClock89 *clock_value,
                                toc89_u32 tick_rate);
void tickoclock89_set_hms(TickOClock89 *clock_value,
                          toc89_i32 hours,
                          toc89_i32 minutes,
                          toc89_i32 seconds,
                          toc89_i32 milliseconds);
void tickoclock89_update(TickOClock89 *clock_value,
                         toc89_u32 delta_ms,
                         toc89_u32 delta_frames);

toc89_u32 tickoclock89_delta_ticks(const TickOClock89 *clock_value);
toc89_u32 tickoclock89_delta_ms(const TickOClock89 *clock_value);
toc89_u32 tickoclock89_delta_frames(const TickOClock89 *clock_value);
toc89_fixed tickoclock89_frametime_q16(const TickOClock89 *clock_value);

toc89_u32 tickoclock89_global_ticks(const TickOClock89 *clock_value);
toc89_u32 tickoclock89_global_ms(const TickOClock89 *clock_value);
toc89_u32 tickoclock89_global_frames(const TickOClock89 *clock_value);
void tickoclock89_get_hms(const TickOClock89 *clock_value,
                          TickOClock89HMS *out_hms);

int tickoclock89_every_ticks(const TickOClock89 *clock_value,
                             toc89_u32 period);
int tickoclock89_every_ms(const TickOClock89 *clock_value,
                          toc89_u32 period);
int tickoclock89_every_frames(const TickOClock89 *clock_value,
                              toc89_u32 period);
int tickoclock89_every_seconds(const TickOClock89 *clock_value,
                               toc89_u32 period);
int tickoclock89_every_minutes(const TickOClock89 *clock_value,
                               toc89_u32 period);

int tickoclock89_unit_from_name(const char *name, int *out_unit);
toc89_u32 tickoclock89_value(const TickOClock89 *clock_value, int unit);

#ifdef __cplusplus
}
#endif

#endif
