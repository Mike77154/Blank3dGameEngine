#include "blank3d_time89.h"
#include "timeverbs89.h"
#include <limits.h>
#include <string.h>

static void b3d_time_status(Blank3DTime89 *t, const char *text)
{
    unsigned int i;
    if (!t) return;
    if (!text) text = "";
    for (i = 0U; i + 1U < (unsigned int)sizeof(t->status) &&
         text[i] != '\0'; ++i)
        t->status[i] = text[i];
    t->status[i] = '\0';
}

static int b3d_u32_to_i32(unsigned int value)
{
    if (value > (unsigned int)INT_MAX) return INT_MAX;
    return (int)value;
}


static int b3d_ms_to_ticks(const Blank3DTime89 *t, unsigned int ms)
{
    unsigned int rate;
    unsigned int whole;
    unsigned int rem;
    unsigned int out;
    if (!t) return 0;
    rate = t->clock.tick_rate ? t->clock.tick_rate : 60U;
    whole = ms / 1000U;
    rem = ms % 1000U;
    if (whole != 0U && rate > (unsigned int)INT_MAX / whole)
        return INT_MAX;
    out = whole * rate;
    if (rem != 0U) {
        unsigned int extra;
        if (rate > UINT_MAX / rem) extra = UINT_MAX;
        else extra = rem * rate;
        if (extra != UINT_MAX) extra = (extra + 999U) / 1000U;
        if (extra > (unsigned int)INT_MAX - out) return INT_MAX;
        out += extra;
    }
    return b3d_u32_to_i32(out);
}

static int b3d_unit_to_ticks(const Blank3DTime89 *t,
                             int unit, unsigned int value)
{
    switch (unit) {
        case TV89_UNIT_TICKS:
            return b3d_u32_to_i32(value);
        case TV89_UNIT_MILLISECONDS:
            return b3d_ms_to_ticks(t, value);
        case TV89_UNIT_SECONDS:
            return timeclocker_seconds_to_ticks(
                (TimeClocker *)&t->timers, b3d_u32_to_i32(value));
        case TV89_UNIT_MINUTES:
            if (value > (unsigned int)INT_MAX / 60U) return INT_MAX;
            return timeclocker_seconds_to_ticks(
                (TimeClocker *)&t->timers, (int)(value * 60U));
        case TV89_UNIT_HOURS:
            if (value > (unsigned int)INT_MAX / 3600U) return INT_MAX;
            return timeclocker_seconds_to_ticks(
                (TimeClocker *)&t->timers, (int)(value * 3600U));
        default:
            return 0;
    }
}

int blank3d_time89_init(Blank3DTime89 *t,
                        const rt_time_source *source,
                        unsigned int max_step_ms,
                        unsigned int tick_rate)
{
    rt_time_config cfg;
    if (!t || !source) return 0;
    memset(t, 0, sizeof(*t));
    rt_time_config_default(&cfg);
    cfg.max_step_ms = max_step_ms;
    cfg.clamp_enabled = max_step_ms != 0U;
    if (rt_time_init_ex(&t->realtime, source, &cfg) != RT_TIME_OK) {
        b3d_time_status(t, rt_time_status_string(
            rt_time_last_status(&t->realtime)));
        return 0;
    }
    tickoclock89_init(&t->clock, tick_rate);
    timeclocker_init(&t->timers, b3d_u32_to_i32(t->clock.tick_rate), 60);
    t->effective_frame_ms = 1U;
    t->initialized = 1;
    b3d_time_status(t, "Blank3D time authority ready");
    return 1;
}

void blank3d_time89_reset(Blank3DTime89 *t)
{
    unsigned int rate;
    if (!t || !t->initialized) return;
    rate = t->clock.tick_rate;
    rt_time_reset(&t->realtime);
    tickoclock89_reset(&t->clock);
    tickoclock89_set_tick_rate(&t->clock, rate);
    timeclocker_init(&t->timers, b3d_u32_to_i32(rate), 60);
    t->effective_frame_ms = rt_time_is_paused(&t->realtime) ? 0U : 1U;
    b3d_time_status(t, "Blank3D time reset");
}

void blank3d_time89_update(Blank3DTime89 *t)
{
    unsigned int dt;
    unsigned int sim_frames;
    if (!t || !t->initialized) return;
    rt_time_update(&t->realtime);
    dt = rt_time_delta_ms(&t->realtime);

    /* Preserve Blank3D's historical minimum 1 ms simulation step while
       running; pause/time-scale zero still produce an actual zero step. */
    if (dt == 0U && !rt_time_is_paused(&t->realtime) &&
        t->realtime.scale_q16 > 0)
        dt = 1U;
    t->effective_frame_ms = dt;
    sim_frames = dt != 0U ? 1U : 0U;

    tickoclock89_update(&t->clock, dt, sim_frames);
    timeclocker_update(&t->timers,
        b3d_u32_to_i32(tickoclock89_delta_ticks(&t->clock)),
        b3d_u32_to_i32(tickoclock89_delta_frames(&t->clock)));
}

unsigned int blank3d_time89_frame_ms(const Blank3DTime89 *t)
{
    return t ? t->effective_frame_ms : 0U;
}
rt_fixed blank3d_time89_delta_q16(const Blank3DTime89 *t)
{
    return t ? rt_time_ms_to_q16_seconds(t->effective_frame_ms) : 0;
}
rt_fixed blank3d_time89_now_q16(const Blank3DTime89 *t)
{
    return t ? rt_time_now_q16(&t->realtime) : 0;
}
unsigned int blank3d_time89_now_ms(const Blank3DTime89 *t)
{
    return t ? rt_time_now_ms(&t->realtime) : 0U;
}
unsigned int blank3d_time89_raw_delta_ms(const Blank3DTime89 *t)
{
    return t ? rt_time_raw_delta_ms(&t->realtime) : 0U;
}

void blank3d_time89_pause(Blank3DTime89 *t)
{
    if (!t) return;
    rt_time_set_paused(&t->realtime, 1);
}
void blank3d_time89_resume(Blank3DTime89 *t)
{
    if (!t) return;
    rt_time_set_paused(&t->realtime, 0);
}
void blank3d_time89_toggle(Blank3DTime89 *t)
{
    if (!t) return;
    rt_time_toggle_paused(&t->realtime);
}
void blank3d_time89_set_scale_q16(Blank3DTime89 *t, int scale_q16)
{
    if (!t) return;
    rt_time_set_scale(&t->realtime, (rt_fixed)scale_q16);
}
void blank3d_time89_set_tick_rate(Blank3DTime89 *t, unsigned int tick_rate)
{
    if (!t) return;
    tickoclock89_set_tick_rate(&t->clock, tick_rate);
    t->timers.ticks_per_second = b3d_u32_to_i32(t->clock.tick_rate);
}
void blank3d_time89_set_hms(Blank3DTime89 *t,
                            int h, int m, int s, int ms)
{
    if (!t) return;
    tickoclock89_set_hms(&t->clock, h, m, s, ms);
}

int blank3d_time89_timer_once(Blank3DTime89 *t,
                              const char *name, int unit, unsigned int value)
{
    int ticks;
    if (!t || !name) return 0;
    if (unit == TV89_UNIT_FRAMES)
        return timeclocker_oneshot_frames(&t->timers, name,
                                          b3d_u32_to_i32(value));
    ticks = b3d_unit_to_ticks(t, unit, value);
    return timeclocker_oneshot_ticks(&t->timers, name, ticks);
}

int blank3d_time89_timer_loop(Blank3DTime89 *t,
                              const char *name, int unit, unsigned int value,
                              int repeat_limit)
{
    int ticks;
    if (!t || !name) return 0;
    if (unit == TV89_UNIT_FRAMES)
        return timeclocker_loop_frames(&t->timers, name,
            b3d_u32_to_i32(value), repeat_limit);
    ticks = b3d_unit_to_ticks(t, unit, value);
    return timeclocker_loop_ticks(&t->timers, name, ticks, repeat_limit);
}

int blank3d_time89_cooldown_set(Blank3DTime89 *t,
                                const char *name, int unit,
                                unsigned int value)
{
    int ticks;
    if (!t || !name) return 0;
    if (unit == TV89_UNIT_FRAMES)
        return timeclocker_cooldown_frames(&t->timers, name,
                                           b3d_u32_to_i32(value));
    ticks = b3d_unit_to_ticks(t, unit, value);
    return timeclocker_cooldown_ticks(&t->timers, name, ticks);
}

int blank3d_time89_stopwatch_start(Blank3DTime89 *t,
                                   const char *name, int unit)
{
    if (!t || !name) return 0;
    return timeclocker_stopwatch_start(&t->timers, name,
        unit == TV89_UNIT_FRAMES ? TIME_CLOCKER_UNIT_FRAMES
                                 : TIME_CLOCKER_UNIT_TICKS);
}

int blank3d_time89_alarm_set(Blank3DTime89 *t,
                             const char *name, int unit, unsigned int value,
                             int action_id, const char *action_name,
                             int a, int b, int c)
{
    int ticks;
    if (!t || !name) return 0;
    if (unit == TV89_UNIT_SECONDS)
        return timeclocker_alarm_seconds(&t->timers, name,
            b3d_u32_to_i32(value), action_id, action_name, a, b, c);
    if (unit == TV89_UNIT_FRAMES)
        ticks = timeclocker_frames_to_ticks(&t->timers,
                                             b3d_u32_to_i32(value));
    else
        ticks = b3d_unit_to_ticks(t, unit, value);
    return timeclocker_alarm_ticks(&t->timers, name, ticks,
                                    action_id, action_name, a, b, c);
}

int blank3d_time89_timer_stop(Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_stop(&t->timers, name) : 0; }
int blank3d_time89_timer_pause(Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_pause(&t->timers, name) : 0; }
int blank3d_time89_timer_resume(Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_resume(&t->timers, name) : 0; }
int blank3d_time89_timer_reset(Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_reset(&t->timers, name) : 0; }
int blank3d_time89_timer_clear(Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_clear(&t->timers, name) : 0; }

int blank3d_time89_timer_exists(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_exists((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_timer_active(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_active((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_timer_done(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_done((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_timer_fired(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_fired((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_timer_ready(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_timer_ready((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_cooldown_ready(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_cooldown_ready((TimeClocker *)&t->timers, name) : 0; }
int blank3d_time89_alarm_pending(const Blank3DTime89 *t, const char *name)
{ return t ? timeclocker_alarm_pending((TimeClocker *)&t->timers, name) : 0; }

int blank3d_time89_every(const Blank3DTime89 *t,
                         int unit, unsigned int period)
{
    if (!t || period == 0U) return 0;
    switch (unit) {
        case TV89_UNIT_TICKS:
            return tickoclock89_every_ticks(&t->clock, period);
        case TV89_UNIT_MILLISECONDS:
            return tickoclock89_every_ms(&t->clock, period);
        case TV89_UNIT_FRAMES:
            return tickoclock89_every_frames(&t->clock, period);
        case TV89_UNIT_SECONDS:
            return tickoclock89_every_seconds(&t->clock, period);
        case TV89_UNIT_MINUTES:
            return tickoclock89_every_minutes(&t->clock, period);
        default:
            return 0;
    }
}

unsigned int blank3d_time89_global(const Blank3DTime89 *t, int unit)
{
    if (!t) return 0U;
    switch (unit) {
        case TV89_UNIT_TICKS:
            return tickoclock89_global_ticks(&t->clock);
        case TV89_UNIT_MILLISECONDS:
            return tickoclock89_global_ms(&t->clock);
        case TV89_UNIT_FRAMES:
            return tickoclock89_global_frames(&t->clock);
        case TV89_UNIT_SECONDS:
            return tickoclock89_global_ms(&t->clock) / 1000U;
        case TV89_UNIT_MINUTES:
            return tickoclock89_global_ms(&t->clock) / 60000U;
        case TV89_UNIT_HOURS:
            return tickoclock89_global_ms(&t->clock) / 3600000U;
        default:
            return 0U;
    }
}

void blank3d_time89_get_hms(const Blank3DTime89 *t,
                            TickOClock89HMS *out_hms)
{
    if (!t) return;
    tickoclock89_get_hms(&t->clock, out_hms);
}
int blank3d_time89_is_paused(const Blank3DTime89 *t)
{
    return t ? rt_time_is_paused(&t->realtime) : 0;
}

const char *blank3d_time89_status(const Blank3DTime89 *t)
{
    return t ? t->status : "Blank3D time unavailable";
}
