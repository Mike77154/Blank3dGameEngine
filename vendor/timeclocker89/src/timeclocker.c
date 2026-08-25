#include "timeclocker.h"
#include <limits.h>

static tc_i32 tc_is_valid_name(const char *name)
{
    if (name == (const char *)0) {
        return TIME_CLOCKER_FALSE;
    }
    if (name[0] == '\0') {
        return TIME_CLOCKER_FALSE;
    }
    return TIME_CLOCKER_TRUE;
}

static void tc_name_clear(char *dst)
{
    tc_i32 i;

    if (dst == (char *)0) {
        return;
    }

    for (i = 0; i < TIME_CLOCKER_NAME_MAX; ++i) {
        dst[i] = '\0';
    }
}

static void tc_name_copy(char *dst, const char *src)
{
    tc_i32 i;

    if (dst == (char *)0) {
        return;
    }

    tc_name_clear(dst);

    if (src == (const char *)0) {
        return;
    }

    for (i = 0; i < TIME_CLOCKER_NAME_MAX - 1; ++i) {
        dst[i] = src[i];
        if (src[i] == '\0') {
            return;
        }
    }

    dst[TIME_CLOCKER_NAME_MAX - 1] = '\0';
}

static tc_i32 tc_name_equal(const char *a, const char *b)
{
    tc_i32 i;

    if (a == (const char *)0 || b == (const char *)0) {
        return TIME_CLOCKER_FALSE;
    }

    for (i = 0; i < TIME_CLOCKER_NAME_MAX; ++i) {
        if (a[i] != b[i]) {
            return TIME_CLOCKER_FALSE;
        }
        if (a[i] == '\0') {
            return TIME_CLOCKER_TRUE;
        }
    }

    return TIME_CLOCKER_TRUE;
}

static tc_i32 tc_sat_mul_nonneg(tc_i32 a, tc_i32 b)
{
    if (a <= 0 || b <= 0) {
        return 0;
    }
    if (a > INT_MAX / b) {
        return INT_MAX;
    }
    return a * b;
}

static tc_i32 tc_sat_add_nonneg(tc_i32 a, tc_i32 b)
{
    if (a < 0) {
        a = 0;
    }
    if (b < 0) {
        b = 0;
    }
    if (a > INT_MAX - b) {
        return INT_MAX;
    }
    return a + b;
}

static tc_i32 tc_div_ceil_nonneg(tc_i32 numerator, tc_i32 denominator)
{
    if (denominator <= 0) {
        return 0;
    }
    if (numerator <= 0) {
        return 0;
    }
    return (numerator + denominator - 1) / denominator;
}

static void tc_timer_clear(TimeClockerTimer *timer)
{
    if (timer == (TimeClockerTimer *)0) {
        return;
    }

    tc_name_clear(timer->name);
    timer->used = 0;
    timer->active = 0;
    timer->paused = 0;
    timer->kind = TIME_CLOCKER_TIMER_EMPTY;
    timer->unit = TIME_CLOCKER_UNIT_TICKS;
    timer->elapsed = 0;
    timer->duration = 0;
    timer->done = 0;
    timer->repeat_count = 0;
    timer->repeat_limit = 0;
    timer->fired_step = 0;
    timer->fire_count = 0;
    timer->pending_action = 0;
    timer->action_id = 0;
    timer->action_a = 0;
    timer->action_b = 0;
    timer->action_c = 0;
    tc_name_clear(timer->action_name);
}

static tc_i32 tc_kind_is_countdown(tc_i32 kind)
{
    if (kind == TIME_CLOCKER_TIMER_ONESHOT) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_LOOP) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_COOLDOWN) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_FRAME_DELAY) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_TICK_DELAY) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_ALARM) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

static tc_i32 tc_kind_is_valid(tc_i32 kind)
{
    if (kind == TIME_CLOCKER_TIMER_ONESHOT) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_LOOP) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_COOLDOWN) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_STOPWATCH) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_FRAME_DELAY) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_TICK_DELAY) {
        return TIME_CLOCKER_TRUE;
    }
    if (kind == TIME_CLOCKER_TIMER_ALARM) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

static tc_i32 tc_unit_is_valid(tc_i32 unit)
{
    if (unit == TIME_CLOCKER_UNIT_TICKS) {
        return TIME_CLOCKER_TRUE;
    }
    if (unit == TIME_CLOCKER_UNIT_FRAMES) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

static tc_i32 tc_acquire_timer_index(TimeClocker *clocker, const char *name)
{
    tc_i32 i;
    tc_i32 found;

    found = timeclocker_find_timer_index(clocker, name);
    if (found >= 0) {
        return found;
    }

    if (clocker == (TimeClocker *)0) {
        return -1;
    }

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        if (!clocker->timers[i].used) {
            return i;
        }
    }

    return -1;
}

static void tc_next_step(TimeClocker *clocker)
{
    tc_i32 i;

    if (clocker == (TimeClocker *)0) {
        return;
    }

    if (clocker->update_step >= INT_MAX - 1) {
        clocker->update_step = 1;
        for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
            clocker->timers[i].fired_step = 0;
        }
    } else {
        clocker->update_step = clocker->update_step + 1;
    }
}

static void tc_update_one_timer(TimeClockerTimer *timer, tc_i32 amount, tc_i32 step)
{
    tc_i32 periods;
    tc_i32 remaining_repeats;

    if (timer == (TimeClockerTimer *)0) {
        return;
    }
    if (!timer->used || !timer->active || timer->paused) {
        return;
    }
    if (amount <= 0) {
        return;
    }

    if (timer->kind == TIME_CLOCKER_TIMER_STOPWATCH) {
        timer->elapsed = tc_sat_add_nonneg(timer->elapsed, amount);
        return;
    }

    if (!tc_kind_is_countdown(timer->kind)) {
        return;
    }

    if (timer->duration <= 0) {
        timer->duration = 1;
    }

    timer->elapsed = tc_sat_add_nonneg(timer->elapsed, amount);

    if (timer->elapsed < timer->duration) {
        return;
    }

    timer->fired_step = step;

    if (timer->kind == TIME_CLOCKER_TIMER_LOOP) {
        periods = timer->elapsed / timer->duration;
        if (periods <= 0) {
            periods = 1;
        }

        if (timer->repeat_limit > 0) {
            remaining_repeats = timer->repeat_limit - timer->repeat_count;
            if (remaining_repeats < 0) {
                remaining_repeats = 0;
            }
            if (periods > remaining_repeats) {
                periods = remaining_repeats;
            }
        }

        if (periods <= 0) {
            timer->active = 0;
            timer->done = 1;
            timer->fire_count = 0;
            timer->elapsed = timer->duration;
            return;
        }

        timer->fire_count = periods;
        timer->repeat_count = tc_sat_add_nonneg(timer->repeat_count, periods);

        if (timer->repeat_limit > 0 && timer->repeat_count >= timer->repeat_limit) {
            timer->active = 0;
            timer->done = 1;
            timer->elapsed = timer->duration;
        } else {
            timer->done = 0;
            timer->elapsed = timer->elapsed % timer->duration;
        }
        return;
    }

    timer->fire_count = 1;
    timer->active = 0;
    timer->done = 1;
    timer->elapsed = timer->duration;

    if (timer->kind == TIME_CLOCKER_TIMER_ALARM) {
        timer->pending_action = 1;
    }
}

void timeclocker_init(TimeClocker *clocker, tc_i32 ticks_per_second, tc_i32 frames_per_second)
{
    tc_i32 i;

    if (clocker == (TimeClocker *)0) {
        return;
    }

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        tc_timer_clear(&clocker->timers[i]);
    }

    if (ticks_per_second <= 0) {
        ticks_per_second = 60;
    }
    if (frames_per_second <= 0) {
        frames_per_second = 60;
    }

    clocker->ticks_per_second = ticks_per_second;
    clocker->frames_per_second = frames_per_second;
    clocker->global_ticks = 0;
    clocker->previous_global_ticks = 0;
    clocker->global_frames = 0;
    clocker->previous_global_frames = 0;
    clocker->update_step = 0;
}

void timeclocker_reset_all(TimeClocker *clocker)
{
    tc_i32 ticks_per_second;
    tc_i32 frames_per_second;

    if (clocker == (TimeClocker *)0) {
        return;
    }

    ticks_per_second = clocker->ticks_per_second;
    frames_per_second = clocker->frames_per_second;
    timeclocker_init(clocker, ticks_per_second, frames_per_second);
}

void timeclocker_update(TimeClocker *clocker, tc_i32 delta_ticks, tc_i32 delta_frames)
{
    tc_i32 i;
    tc_i32 amount;

    if (clocker == (TimeClocker *)0) {
        return;
    }

    if (delta_ticks < 0) {
        delta_ticks = 0;
    }
    if (delta_frames < 0) {
        delta_frames = 0;
    }

    tc_next_step(clocker);

    clocker->previous_global_ticks = clocker->global_ticks;
    clocker->previous_global_frames = clocker->global_frames;
    clocker->global_ticks = tc_sat_add_nonneg(clocker->global_ticks, delta_ticks);
    clocker->global_frames = tc_sat_add_nonneg(clocker->global_frames, delta_frames);

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        if (clocker->timers[i].unit == TIME_CLOCKER_UNIT_TICKS) {
            amount = delta_ticks;
        } else {
            amount = delta_frames;
        }
        tc_update_one_timer(&clocker->timers[i], amount, clocker->update_step);
    }
}

void timeclocker_update_ticks(TimeClocker *clocker, tc_i32 delta_ticks)
{
    timeclocker_update(clocker, delta_ticks, 0);
}

void timeclocker_update_frame(TimeClocker *clocker)
{
    timeclocker_update(clocker, 0, 1);
}

tc_fixed timeclocker_fixed_from_int(tc_i32 value)
{
    if (value > INT_MAX / TIME_CLOCKER_FP_ONE) {
        return INT_MAX;
    }
    if (value < INT_MIN / TIME_CLOCKER_FP_ONE) {
        return INT_MIN;
    }
    return value * TIME_CLOCKER_FP_ONE;
}

tc_i32 timeclocker_fixed_to_int_floor(tc_fixed value)
{
    return value / TIME_CLOCKER_FP_ONE;
}

tc_i32 timeclocker_fixed_to_int_ceil(tc_fixed value)
{
    tc_i32 whole;
    tc_i32 rem;

    whole = value / TIME_CLOCKER_FP_ONE;
    rem = value % TIME_CLOCKER_FP_ONE;
    if (rem > 0) {
        return whole + 1;
    }
    return whole;
}

tc_fixed timeclocker_fixed_fraction(tc_i32 numerator, tc_i32 denominator)
{
    tc_i32 whole;
    tc_i32 rem;
    tc_i32 frac;

    if (denominator <= 0) {
        return 0;
    }
    if (numerator <= 0) {
        return 0;
    }

    whole = numerator / denominator;
    rem = numerator % denominator;
    frac = tc_sat_mul_nonneg(rem, TIME_CLOCKER_FP_ONE) / denominator;
    return tc_sat_add_nonneg(timeclocker_fixed_from_int(whole), frac);
}

tc_i32 timeclocker_seconds_to_ticks(TimeClocker *clocker, tc_i32 seconds)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (seconds <= 0) {
        return 0;
    }
    return tc_sat_mul_nonneg(seconds, clocker->ticks_per_second);
}

tc_i32 timeclocker_minutes_to_ticks(TimeClocker *clocker, tc_i32 minutes)
{
    tc_i32 seconds;

    if (minutes <= 0) {
        return 0;
    }
    seconds = tc_sat_mul_nonneg(minutes, 60);
    return timeclocker_seconds_to_ticks(clocker, seconds);
}

tc_i32 timeclocker_frames_to_ticks(TimeClocker *clocker, tc_i32 frames)
{
    tc_i32 product;

    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (frames <= 0 || clocker->frames_per_second <= 0) {
        return 0;
    }

    product = tc_sat_mul_nonneg(frames, clocker->ticks_per_second);
    return tc_div_ceil_nonneg(product, clocker->frames_per_second);
}

tc_i32 timeclocker_ticks_to_frames(TimeClocker *clocker, tc_i32 ticks)
{
    tc_i32 product;

    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (ticks <= 0 || clocker->ticks_per_second <= 0) {
        return 0;
    }

    product = tc_sat_mul_nonneg(ticks, clocker->frames_per_second);
    return tc_div_ceil_nonneg(product, clocker->ticks_per_second);
}

tc_i32 timeclocker_seconds_fixed_to_ticks(TimeClocker *clocker, tc_fixed seconds_fixed)
{
    tc_i32 whole;
    tc_i32 frac;
    tc_i32 ticks;
    tc_i32 extra_num;
    tc_i32 extra;

    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (seconds_fixed <= 0) {
        return 0;
    }

    whole = seconds_fixed / TIME_CLOCKER_FP_ONE;
    frac = seconds_fixed % TIME_CLOCKER_FP_ONE;
    ticks = tc_sat_mul_nonneg(whole, clocker->ticks_per_second);
    extra_num = tc_sat_mul_nonneg(frac, clocker->ticks_per_second);
    extra = tc_div_ceil_nonneg(extra_num, TIME_CLOCKER_FP_ONE);
    return tc_sat_add_nonneg(ticks, extra);
}

tc_fixed timeclocker_ticks_to_seconds_fixed(TimeClocker *clocker, tc_i32 ticks)
{
    tc_i32 whole;
    tc_i32 rem;
    tc_i32 frac;

    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (ticks <= 0 || clocker->ticks_per_second <= 0) {
        return 0;
    }

    whole = ticks / clocker->ticks_per_second;
    rem = ticks % clocker->ticks_per_second;
    frac = tc_sat_mul_nonneg(rem, TIME_CLOCKER_FP_ONE) / clocker->ticks_per_second;
    return tc_sat_add_nonneg(timeclocker_fixed_from_int(whole), frac);
}

tc_i32 timeclocker_ticks_to_seconds_floor(TimeClocker *clocker, tc_i32 ticks)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    if (ticks <= 0 || clocker->ticks_per_second <= 0) {
        return 0;
    }
    return ticks / clocker->ticks_per_second;
}

tc_i32 timeclocker_ticks_to_minutes_floor(TimeClocker *clocker, tc_i32 ticks)
{
    return timeclocker_ticks_to_seconds_floor(clocker, ticks) / 60;
}

tc_i32 timeclocker_global_ticks(TimeClocker *clocker)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    return clocker->global_ticks;
}

tc_i32 timeclocker_global_frames(TimeClocker *clocker)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    return clocker->global_frames;
}

tc_i32 timeclocker_global_seconds(TimeClocker *clocker)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    return timeclocker_ticks_to_seconds_floor(clocker, clocker->global_ticks);
}

tc_i32 timeclocker_global_minutes(TimeClocker *clocker)
{
    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    return timeclocker_ticks_to_minutes_floor(clocker, clocker->global_ticks);
}

tc_i32 timeclocker_find_timer_index(TimeClocker *clocker, const char *name)
{
    tc_i32 i;

    if (clocker == (TimeClocker *)0 || !tc_is_valid_name(name)) {
        return -1;
    }

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        if (clocker->timers[i].used && tc_name_equal(clocker->timers[i].name, name)) {
            return i;
        }
    }

    return -1;
}

TimeClockerTimer *timeclocker_get_timer(TimeClocker *clocker, const char *name)
{
    tc_i32 index;

    index = timeclocker_find_timer_index(clocker, name);
    if (index < 0) {
        return (TimeClockerTimer *)0;
    }
    return &clocker->timers[index];
}

tc_i32 timeclocker_start(TimeClocker *clocker, const char *name, tc_i32 kind, tc_i32 unit, tc_i32 duration, tc_i32 repeat_limit)
{
    tc_i32 index;
    TimeClockerTimer *timer;

    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_ERR;
    }
    if (!tc_is_valid_name(name)) {
        return TIME_CLOCKER_ERR;
    }
    if (!tc_kind_is_valid(kind)) {
        return TIME_CLOCKER_ERR;
    }
    if (!tc_unit_is_valid(unit)) {
        return TIME_CLOCKER_ERR;
    }

    if (kind == TIME_CLOCKER_TIMER_FRAME_DELAY) {
        unit = TIME_CLOCKER_UNIT_FRAMES;
    }
    if (kind == TIME_CLOCKER_TIMER_TICK_DELAY) {
        unit = TIME_CLOCKER_UNIT_TICKS;
    }

    if (kind != TIME_CLOCKER_TIMER_STOPWATCH && duration <= 0) {
        duration = 1;
    }

    index = tc_acquire_timer_index(clocker, name);
    if (index < 0) {
        return TIME_CLOCKER_ERR;
    }

    timer = &clocker->timers[index];
    tc_timer_clear(timer);
    tc_name_copy(timer->name, name);
    timer->used = 1;
    timer->active = 1;
    timer->paused = 0;
    timer->kind = kind;
    timer->unit = unit;
    timer->elapsed = 0;
    timer->duration = duration;
    timer->done = 0;
    timer->repeat_count = 0;
    timer->repeat_limit = repeat_limit;
    timer->fired_step = 0;
    timer->fire_count = 0;
    timer->pending_action = 0;

    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_oneshot_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_ONESHOT, TIME_CLOCKER_UNIT_TICKS, ticks, 0);
}

tc_i32 timeclocker_oneshot_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds)
{
    return timeclocker_oneshot_ticks(clocker, name, timeclocker_seconds_to_ticks(clocker, seconds));
}

tc_i32 timeclocker_oneshot_frames(TimeClocker *clocker, const char *name, tc_i32 frames)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_ONESHOT, TIME_CLOCKER_UNIT_FRAMES, frames, 0);
}

tc_i32 timeclocker_loop_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks, tc_i32 repeat_limit)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_LOOP, TIME_CLOCKER_UNIT_TICKS, ticks, repeat_limit);
}

tc_i32 timeclocker_loop_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds, tc_i32 repeat_limit)
{
    return timeclocker_loop_ticks(clocker, name, timeclocker_seconds_to_ticks(clocker, seconds), repeat_limit);
}

tc_i32 timeclocker_loop_frames(TimeClocker *clocker, const char *name, tc_i32 frames, tc_i32 repeat_limit)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_LOOP, TIME_CLOCKER_UNIT_FRAMES, frames, repeat_limit);
}

tc_i32 timeclocker_cooldown_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_COOLDOWN, TIME_CLOCKER_UNIT_TICKS, ticks, 0);
}

tc_i32 timeclocker_cooldown_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds)
{
    return timeclocker_cooldown_ticks(clocker, name, timeclocker_seconds_to_ticks(clocker, seconds));
}

tc_i32 timeclocker_cooldown_frames(TimeClocker *clocker, const char *name, tc_i32 frames)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_COOLDOWN, TIME_CLOCKER_UNIT_FRAMES, frames, 0);
}

tc_i32 timeclocker_stopwatch_start(TimeClocker *clocker, const char *name, tc_i32 unit)
{
    if (!tc_unit_is_valid(unit)) {
        unit = TIME_CLOCKER_UNIT_TICKS;
    }
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_STOPWATCH, unit, 0, 0);
}

tc_i32 timeclocker_frame_delay(TimeClocker *clocker, const char *name, tc_i32 frames)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_FRAME_DELAY, TIME_CLOCKER_UNIT_FRAMES, frames, 0);
}

tc_i32 timeclocker_tick_delay(TimeClocker *clocker, const char *name, tc_i32 ticks)
{
    return timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_TICK_DELAY, TIME_CLOCKER_UNIT_TICKS, ticks, 0);
}

tc_i32 timeclocker_alarm_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks, tc_i32 action_id, const char *action_name, tc_i32 a, tc_i32 b, tc_i32 c)
{
    TimeClockerTimer *timer;

    if (!timeclocker_start(clocker, name, TIME_CLOCKER_TIMER_ALARM, TIME_CLOCKER_UNIT_TICKS, ticks, 0)) {
        return TIME_CLOCKER_ERR;
    }

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }

    timer->action_id = action_id;
    timer->action_a = a;
    timer->action_b = b;
    timer->action_c = c;
    tc_name_copy(timer->action_name, action_name);

    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_alarm_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds, tc_i32 action_id, const char *action_name, tc_i32 a, tc_i32 b, tc_i32 c)
{
    return timeclocker_alarm_ticks(clocker, name, timeclocker_seconds_to_ticks(clocker, seconds), action_id, action_name, a, b, c);
}

tc_i32 timeclocker_timer_stop(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }
    timer->active = 0;
    timer->paused = 0;
    timer->done = 0;
    timer->pending_action = 0;
    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_timer_pause(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }
    if (timer->active) {
        timer->paused = 1;
    }
    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_timer_resume(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }
    timer->paused = 0;
    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_timer_reset(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }
    timer->elapsed = 0;
    timer->done = 0;
    timer->paused = 0;
    timer->active = 1;
    timer->repeat_count = 0;
    timer->fired_step = 0;
    timer->fire_count = 0;
    timer->pending_action = 0;
    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_timer_clear(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_ERR;
    }
    tc_timer_clear(timer);
    return TIME_CLOCKER_OK;
}

tc_i32 timeclocker_timer_exists(TimeClocker *clocker, const char *name)
{
    return timeclocker_find_timer_index(clocker, name) >= 0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_timer_active(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_FALSE;
    }
    return timer->active ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_timer_paused(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_FALSE;
    }
    return timer->paused ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_timer_done(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_FALSE;
    }
    return timer->done ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_timer_fired(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_FALSE;
    }
    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_FALSE;
    }
    if (timer->fired_step == clocker->update_step && timer->fire_count > 0) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_timer_ready(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return TIME_CLOCKER_TRUE;
    }
    if (!timer->active) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_cooldown_ready(TimeClocker *clocker, const char *name)
{
    return timeclocker_timer_ready(clocker, name);
}

tc_i32 timeclocker_timer_fire_count(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    if (clocker == (TimeClocker *)0) {
        return 0;
    }
    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return 0;
    }
    if (timer->fired_step == clocker->update_step) {
        return timer->fire_count;
    }
    return 0;
}

tc_i32 timeclocker_timer_elapsed_units(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return 0;
    }
    return timer->elapsed;
}

tc_i32 timeclocker_timer_remaining_units(TimeClocker *clocker, const char *name)
{
    TimeClockerTimer *timer;

    timer = timeclocker_get_timer(clocker, name);
    if (timer == (TimeClockerTimer *)0) {
        return 0;
    }
    if (timer->kind == TIME_CLOCKER_TIMER_STOPWATCH) {
        return 0;
    }
    if (timer->done || !timer->active) {
        return 0;
    }
    if (timer->elapsed >= timer->duration) {
        return 0;
    }
    return timer->duration - timer->elapsed;
}

tc_i32 timeclocker_every_ticks(TimeClocker *clocker, tc_i32 ticks)
{
    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_FALSE;
    }
    if (ticks <= 0) {
        return TIME_CLOCKER_FALSE;
    }
    if (clocker->global_ticks <= clocker->previous_global_ticks) {
        return TIME_CLOCKER_FALSE;
    }
    if ((clocker->global_ticks / ticks) != (clocker->previous_global_ticks / ticks)) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_every_frames(TimeClocker *clocker, tc_i32 frames)
{
    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_FALSE;
    }
    if (frames <= 0) {
        return TIME_CLOCKER_FALSE;
    }
    if (clocker->global_frames <= clocker->previous_global_frames) {
        return TIME_CLOCKER_FALSE;
    }
    if ((clocker->global_frames / frames) != (clocker->previous_global_frames / frames)) {
        return TIME_CLOCKER_TRUE;
    }
    return TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_every_seconds(TimeClocker *clocker, tc_i32 seconds)
{
    return timeclocker_every_ticks(clocker, timeclocker_seconds_to_ticks(clocker, seconds));
}

tc_i32 timeclocker_every_minutes(TimeClocker *clocker, tc_i32 minutes)
{
    return timeclocker_every_ticks(clocker, timeclocker_minutes_to_ticks(clocker, minutes));
}

void timeclocker_condition_init(TimeClockerCondition *condition, tc_i32 kind, const char *name, tc_i32 value0, tc_i32 value1, tc_i32 value2)
{
    if (condition == (TimeClockerCondition *)0) {
        return;
    }
    condition->kind = kind;
    tc_name_copy(condition->name, name);
    condition->value0 = value0;
    condition->value1 = value1;
    condition->value2 = value2;
}

tc_i32 timeclocker_condition_eval(TimeClocker *clocker, const TimeClockerCondition *condition)
{
    tc_i32 elapsed;
    tc_i32 remaining;

    if (condition == (const TimeClockerCondition *)0) {
        return TIME_CLOCKER_FALSE;
    }

    switch (condition->kind) {
    case TIME_CLOCKER_COND_ALWAYS:
        return TIME_CLOCKER_TRUE;
    case TIME_CLOCKER_COND_TIMER_EXISTS:
        return timeclocker_timer_exists(clocker, condition->name);
    case TIME_CLOCKER_COND_TIMER_ACTIVE:
        return timeclocker_timer_active(clocker, condition->name);
    case TIME_CLOCKER_COND_TIMER_DONE:
        return timeclocker_timer_done(clocker, condition->name);
    case TIME_CLOCKER_COND_TIMER_FIRED:
        return timeclocker_timer_fired(clocker, condition->name);
    case TIME_CLOCKER_COND_TIMER_READY:
        return timeclocker_timer_ready(clocker, condition->name);
    case TIME_CLOCKER_COND_COOLDOWN_READY:
        return timeclocker_cooldown_ready(clocker, condition->name);
    case TIME_CLOCKER_COND_EVERY_TICKS:
        return timeclocker_every_ticks(clocker, condition->value0);
    case TIME_CLOCKER_COND_EVERY_FRAMES:
        return timeclocker_every_frames(clocker, condition->value0);
    case TIME_CLOCKER_COND_EVERY_SECONDS:
        return timeclocker_every_seconds(clocker, condition->value0);
    case TIME_CLOCKER_COND_EVERY_MINUTES:
        return timeclocker_every_minutes(clocker, condition->value0);
    case TIME_CLOCKER_COND_TIMER_ELAPSED_GTE:
        elapsed = timeclocker_timer_elapsed_units(clocker, condition->name);
        return elapsed >= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    case TIME_CLOCKER_COND_TIMER_REMAINING_LTE:
        remaining = timeclocker_timer_remaining_units(clocker, condition->name);
        return remaining <= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    case TIME_CLOCKER_COND_ALARM_PENDING:
        return timeclocker_alarm_pending(clocker, condition->name);
    case TIME_CLOCKER_COND_GLOBAL_TICKS_GTE:
        return timeclocker_global_ticks(clocker) >= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    case TIME_CLOCKER_COND_GLOBAL_FRAMES_GTE:
        return timeclocker_global_frames(clocker) >= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    case TIME_CLOCKER_COND_GLOBAL_SECONDS_GTE:
        return timeclocker_global_seconds(clocker) >= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    case TIME_CLOCKER_COND_GLOBAL_MINUTES_GTE:
        return timeclocker_global_minutes(clocker) >= condition->value0 ? TIME_CLOCKER_TRUE : TIME_CLOCKER_FALSE;
    default:
        break;
    }

    return TIME_CLOCKER_FALSE;
}

void timeclocker_action_init(TimeClockerAction *action, tc_i32 kind, const char *name, tc_i32 value0, tc_i32 value1, tc_i32 value2)
{
    if (action == (TimeClockerAction *)0) {
        return;
    }
    action->kind = kind;
    tc_name_copy(action->name, name);
    action->value0 = value0;
    action->value1 = value1;
    action->value2 = value2;
    action->event_id = 0;
    tc_name_clear(action->event_name);
}

void timeclocker_action_set_event(TimeClockerAction *action, tc_i32 event_id, const char *event_name)
{
    if (action == (TimeClockerAction *)0) {
        return;
    }
    action->event_id = event_id;
    tc_name_copy(action->event_name, event_name);
}

tc_i32 timeclocker_action_run(TimeClocker *clocker, const TimeClockerAction *action)
{
    if (action == (const TimeClockerAction *)0) {
        return TIME_CLOCKER_ERR;
    }

    switch (action->kind) {
    case TIME_CLOCKER_ACT_NONE:
        return TIME_CLOCKER_OK;
    case TIME_CLOCKER_ACT_TIMER_ONESHOT_TICKS:
        return timeclocker_oneshot_ticks(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_TIMER_ONESHOT_SECONDS:
        return timeclocker_oneshot_seconds(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_TIMER_ONESHOT_FRAMES:
        return timeclocker_oneshot_frames(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_TIMER_LOOP_TICKS:
        return timeclocker_loop_ticks(clocker, action->name, action->value0, action->value1);
    case TIME_CLOCKER_ACT_TIMER_LOOP_SECONDS:
        return timeclocker_loop_seconds(clocker, action->name, action->value0, action->value1);
    case TIME_CLOCKER_ACT_TIMER_LOOP_FRAMES:
        return timeclocker_loop_frames(clocker, action->name, action->value0, action->value1);
    case TIME_CLOCKER_ACT_COOLDOWN_TICKS:
        return timeclocker_cooldown_ticks(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_COOLDOWN_SECONDS:
        return timeclocker_cooldown_seconds(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_COOLDOWN_FRAMES:
        return timeclocker_cooldown_frames(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_STOPWATCH_START:
        return timeclocker_stopwatch_start(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_FRAME_DELAY:
        return timeclocker_frame_delay(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_TICK_DELAY:
        return timeclocker_tick_delay(clocker, action->name, action->value0);
    case TIME_CLOCKER_ACT_ALARM_TICKS:
        return timeclocker_alarm_ticks(clocker, action->name, action->value0, action->event_id, action->event_name, action->value1, action->value2, 0);
    case TIME_CLOCKER_ACT_ALARM_SECONDS:
        return timeclocker_alarm_seconds(clocker, action->name, action->value0, action->event_id, action->event_name, action->value1, action->value2, 0);
    case TIME_CLOCKER_ACT_TIMER_STOP:
        return timeclocker_timer_stop(clocker, action->name);
    case TIME_CLOCKER_ACT_TIMER_PAUSE:
        return timeclocker_timer_pause(clocker, action->name);
    case TIME_CLOCKER_ACT_TIMER_RESUME:
        return timeclocker_timer_resume(clocker, action->name);
    case TIME_CLOCKER_ACT_TIMER_RESET:
        return timeclocker_timer_reset(clocker, action->name);
    case TIME_CLOCKER_ACT_TIMER_CLEAR:
        return timeclocker_timer_clear(clocker, action->name);
    default:
        break;
    }

    return TIME_CLOCKER_ERR;
}

void timeclocker_event_init(TimeClockerEvent *event_value)
{
    if (event_value == (TimeClockerEvent *)0) {
        return;
    }
    event_value->valid = 0;
    tc_name_clear(event_value->timer_name);
    event_value->action_id = 0;
    event_value->action_a = 0;
    event_value->action_b = 0;
    event_value->action_c = 0;
    tc_name_clear(event_value->action_name);
}

tc_i32 timeclocker_alarm_pending(TimeClocker *clocker, const char *name)
{
    tc_i32 i;
    TimeClockerTimer *timer;

    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_FALSE;
    }

    if (tc_is_valid_name(name)) {
        timer = timeclocker_get_timer(clocker, name);
        if (timer == (TimeClockerTimer *)0) {
            return TIME_CLOCKER_FALSE;
        }
        if (timer->kind == TIME_CLOCKER_TIMER_ALARM && timer->pending_action) {
            return TIME_CLOCKER_TRUE;
        }
        return TIME_CLOCKER_FALSE;
    }

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        timer = &clocker->timers[i];
        if (timer->used && timer->kind == TIME_CLOCKER_TIMER_ALARM && timer->pending_action) {
            return TIME_CLOCKER_TRUE;
        }
    }

    return TIME_CLOCKER_FALSE;
}

tc_i32 timeclocker_poll_alarm(TimeClocker *clocker, TimeClockerEvent *event_value, tc_i32 consume)
{
    tc_i32 i;
    TimeClockerTimer *timer;

    if (event_value != (TimeClockerEvent *)0) {
        timeclocker_event_init(event_value);
    }

    if (clocker == (TimeClocker *)0) {
        return TIME_CLOCKER_FALSE;
    }

    for (i = 0; i < TIME_CLOCKER_MAX_TIMERS; ++i) {
        timer = &clocker->timers[i];
        if (timer->used && timer->kind == TIME_CLOCKER_TIMER_ALARM && timer->pending_action) {
            if (event_value != (TimeClockerEvent *)0) {
                event_value->valid = 1;
                tc_name_copy(event_value->timer_name, timer->name);
                event_value->action_id = timer->action_id;
                event_value->action_a = timer->action_a;
                event_value->action_b = timer->action_b;
                event_value->action_c = timer->action_c;
                tc_name_copy(event_value->action_name, timer->action_name);
            }
            if (consume) {
                timer->pending_action = 0;
            }
            return TIME_CLOCKER_TRUE;
        }
    }

    return TIME_CLOCKER_FALSE;
}
