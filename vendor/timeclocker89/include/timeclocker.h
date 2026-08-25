#ifndef TIME_CLOCKER_H
#define TIME_CLOCKER_H

/*
    timeclocker.h

    C89 game timer/cooldown/clock binding layer.

    Rules:
    - no dynamic allocation calls
    - no console/file I/O header
    - integer and fixed-point only
    - no C99-only headers or syntax
*/

#include <stddef.h>
#include <limits.h>

#if INT_MAX < 2147483647
#error timeclocker89 requires an at-least-32-bit signed int
#endif
#if UINT_MAX < 4294967295U
#error timeclocker89 requires an at-least-32-bit unsigned int
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIME_CLOCKER_MAX_TIMERS
#define TIME_CLOCKER_MAX_TIMERS 128
#endif

#ifndef TIME_CLOCKER_NAME_MAX
#define TIME_CLOCKER_NAME_MAX 32
#endif

#define TIME_CLOCKER_FALSE 0
#define TIME_CLOCKER_TRUE  1
#define TIME_CLOCKER_OK    1
#define TIME_CLOCKER_ERR   0

#define TIME_CLOCKER_FP_SHIFT 16
#define TIME_CLOCKER_FP_ONE   ((tc_i32)65536)

typedef signed int tc_i32;
typedef unsigned int tc_u32;
typedef tc_i32 tc_fixed;

typedef enum TimeClockerTimerKind {
    TIME_CLOCKER_TIMER_EMPTY = 0,
    TIME_CLOCKER_TIMER_ONESHOT = 1,
    TIME_CLOCKER_TIMER_LOOP = 2,
    TIME_CLOCKER_TIMER_COOLDOWN = 3,
    TIME_CLOCKER_TIMER_STOPWATCH = 4,
    TIME_CLOCKER_TIMER_FRAME_DELAY = 5,
    TIME_CLOCKER_TIMER_TICK_DELAY = 6,
    TIME_CLOCKER_TIMER_ALARM = 7
} TimeClockerTimerKind;

typedef enum TimeClockerUnit {
    TIME_CLOCKER_UNIT_TICKS = 1,
    TIME_CLOCKER_UNIT_FRAMES = 2
} TimeClockerUnit;

typedef enum TimeClockerConditionKind {
    TIME_CLOCKER_COND_ALWAYS = 1,
    TIME_CLOCKER_COND_TIMER_EXISTS = 2,
    TIME_CLOCKER_COND_TIMER_ACTIVE = 3,
    TIME_CLOCKER_COND_TIMER_DONE = 4,
    TIME_CLOCKER_COND_TIMER_FIRED = 5,
    TIME_CLOCKER_COND_TIMER_READY = 6,
    TIME_CLOCKER_COND_COOLDOWN_READY = 7,
    TIME_CLOCKER_COND_EVERY_TICKS = 8,
    TIME_CLOCKER_COND_EVERY_FRAMES = 9,
    TIME_CLOCKER_COND_EVERY_SECONDS = 10,
    TIME_CLOCKER_COND_EVERY_MINUTES = 11,
    TIME_CLOCKER_COND_TIMER_ELAPSED_GTE = 12,
    TIME_CLOCKER_COND_TIMER_REMAINING_LTE = 13,
    TIME_CLOCKER_COND_ALARM_PENDING = 14,
    TIME_CLOCKER_COND_GLOBAL_TICKS_GTE = 15,
    TIME_CLOCKER_COND_GLOBAL_FRAMES_GTE = 16,
    TIME_CLOCKER_COND_GLOBAL_SECONDS_GTE = 17,
    TIME_CLOCKER_COND_GLOBAL_MINUTES_GTE = 18
} TimeClockerConditionKind;

typedef enum TimeClockerActionKind {
    TIME_CLOCKER_ACT_NONE = 0,
    TIME_CLOCKER_ACT_TIMER_ONESHOT_TICKS = 1,
    TIME_CLOCKER_ACT_TIMER_ONESHOT_SECONDS = 2,
    TIME_CLOCKER_ACT_TIMER_ONESHOT_FRAMES = 3,
    TIME_CLOCKER_ACT_TIMER_LOOP_TICKS = 4,
    TIME_CLOCKER_ACT_TIMER_LOOP_SECONDS = 5,
    TIME_CLOCKER_ACT_TIMER_LOOP_FRAMES = 6,
    TIME_CLOCKER_ACT_COOLDOWN_TICKS = 7,
    TIME_CLOCKER_ACT_COOLDOWN_SECONDS = 8,
    TIME_CLOCKER_ACT_COOLDOWN_FRAMES = 9,
    TIME_CLOCKER_ACT_STOPWATCH_START = 10,
    TIME_CLOCKER_ACT_FRAME_DELAY = 11,
    TIME_CLOCKER_ACT_TICK_DELAY = 12,
    TIME_CLOCKER_ACT_ALARM_TICKS = 13,
    TIME_CLOCKER_ACT_ALARM_SECONDS = 14,
    TIME_CLOCKER_ACT_TIMER_STOP = 15,
    TIME_CLOCKER_ACT_TIMER_PAUSE = 16,
    TIME_CLOCKER_ACT_TIMER_RESUME = 17,
    TIME_CLOCKER_ACT_TIMER_RESET = 18,
    TIME_CLOCKER_ACT_TIMER_CLEAR = 19
} TimeClockerActionKind;

typedef struct TimeClockerTimer {
    char name[TIME_CLOCKER_NAME_MAX];

    tc_i32 used;
    tc_i32 active;
    tc_i32 paused;
    tc_i32 kind;
    tc_i32 unit;

    tc_i32 elapsed;
    tc_i32 duration;

    tc_i32 done;
    tc_i32 repeat_count;
    tc_i32 repeat_limit;

    tc_i32 fired_step;
    tc_i32 fire_count;

    tc_i32 pending_action;
    tc_i32 action_id;
    tc_i32 action_a;
    tc_i32 action_b;
    tc_i32 action_c;
    char action_name[TIME_CLOCKER_NAME_MAX];
} TimeClockerTimer;

typedef struct TimeClocker {
    TimeClockerTimer timers[TIME_CLOCKER_MAX_TIMERS];

    tc_i32 ticks_per_second;
    tc_i32 frames_per_second;

    tc_i32 global_ticks;
    tc_i32 previous_global_ticks;

    tc_i32 global_frames;
    tc_i32 previous_global_frames;

    tc_i32 update_step;
} TimeClocker;

typedef struct TimeClockerCondition {
    tc_i32 kind;
    char name[TIME_CLOCKER_NAME_MAX];
    tc_i32 value0;
    tc_i32 value1;
    tc_i32 value2;
} TimeClockerCondition;

typedef struct TimeClockerAction {
    tc_i32 kind;
    char name[TIME_CLOCKER_NAME_MAX];
    tc_i32 value0;
    tc_i32 value1;
    tc_i32 value2;
    tc_i32 event_id;
    char event_name[TIME_CLOCKER_NAME_MAX];
} TimeClockerAction;

typedef struct TimeClockerEvent {
    tc_i32 valid;
    char timer_name[TIME_CLOCKER_NAME_MAX];
    tc_i32 action_id;
    tc_i32 action_a;
    tc_i32 action_b;
    tc_i32 action_c;
    char action_name[TIME_CLOCKER_NAME_MAX];
} TimeClockerEvent;

void timeclocker_init(TimeClocker *clocker, tc_i32 ticks_per_second, tc_i32 frames_per_second);
void timeclocker_reset_all(TimeClocker *clocker);
void timeclocker_update(TimeClocker *clocker, tc_i32 delta_ticks, tc_i32 delta_frames);
void timeclocker_update_ticks(TimeClocker *clocker, tc_i32 delta_ticks);
void timeclocker_update_frame(TimeClocker *clocker);

/* Fixed point helpers, Q16.16. */
tc_fixed timeclocker_fixed_from_int(tc_i32 value);
tc_i32 timeclocker_fixed_to_int_floor(tc_fixed value);
tc_i32 timeclocker_fixed_to_int_ceil(tc_fixed value);
tc_fixed timeclocker_fixed_fraction(tc_i32 numerator, tc_i32 denominator);

/* Time conversion helpers. */
tc_i32 timeclocker_seconds_to_ticks(TimeClocker *clocker, tc_i32 seconds);
tc_i32 timeclocker_minutes_to_ticks(TimeClocker *clocker, tc_i32 minutes);
tc_i32 timeclocker_frames_to_ticks(TimeClocker *clocker, tc_i32 frames);
tc_i32 timeclocker_ticks_to_frames(TimeClocker *clocker, tc_i32 ticks);
tc_i32 timeclocker_seconds_fixed_to_ticks(TimeClocker *clocker, tc_fixed seconds_fixed);
tc_fixed timeclocker_ticks_to_seconds_fixed(TimeClocker *clocker, tc_i32 ticks);
tc_i32 timeclocker_ticks_to_seconds_floor(TimeClocker *clocker, tc_i32 ticks);
tc_i32 timeclocker_ticks_to_minutes_floor(TimeClocker *clocker, tc_i32 ticks);

tc_i32 timeclocker_global_ticks(TimeClocker *clocker);
tc_i32 timeclocker_global_frames(TimeClocker *clocker);
tc_i32 timeclocker_global_seconds(TimeClocker *clocker);
tc_i32 timeclocker_global_minutes(TimeClocker *clocker);

tc_i32 timeclocker_find_timer_index(TimeClocker *clocker, const char *name);
TimeClockerTimer *timeclocker_get_timer(TimeClocker *clocker, const char *name);

tc_i32 timeclocker_start(TimeClocker *clocker, const char *name, tc_i32 kind, tc_i32 unit, tc_i32 duration, tc_i32 repeat_limit);
tc_i32 timeclocker_oneshot_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks);
tc_i32 timeclocker_oneshot_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds);
tc_i32 timeclocker_oneshot_frames(TimeClocker *clocker, const char *name, tc_i32 frames);
tc_i32 timeclocker_loop_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks, tc_i32 repeat_limit);
tc_i32 timeclocker_loop_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds, tc_i32 repeat_limit);
tc_i32 timeclocker_loop_frames(TimeClocker *clocker, const char *name, tc_i32 frames, tc_i32 repeat_limit);
tc_i32 timeclocker_cooldown_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks);
tc_i32 timeclocker_cooldown_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds);
tc_i32 timeclocker_cooldown_frames(TimeClocker *clocker, const char *name, tc_i32 frames);
tc_i32 timeclocker_stopwatch_start(TimeClocker *clocker, const char *name, tc_i32 unit);
tc_i32 timeclocker_frame_delay(TimeClocker *clocker, const char *name, tc_i32 frames);
tc_i32 timeclocker_tick_delay(TimeClocker *clocker, const char *name, tc_i32 ticks);
tc_i32 timeclocker_alarm_ticks(TimeClocker *clocker, const char *name, tc_i32 ticks, tc_i32 action_id, const char *action_name, tc_i32 a, tc_i32 b, tc_i32 c);
tc_i32 timeclocker_alarm_seconds(TimeClocker *clocker, const char *name, tc_i32 seconds, tc_i32 action_id, const char *action_name, tc_i32 a, tc_i32 b, tc_i32 c);

tc_i32 timeclocker_timer_stop(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_pause(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_resume(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_reset(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_clear(TimeClocker *clocker, const char *name);

tc_i32 timeclocker_timer_exists(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_active(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_paused(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_done(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_fired(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_ready(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_cooldown_ready(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_fire_count(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_elapsed_units(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_timer_remaining_units(TimeClocker *clocker, const char *name);

tc_i32 timeclocker_every_ticks(TimeClocker *clocker, tc_i32 ticks);
tc_i32 timeclocker_every_frames(TimeClocker *clocker, tc_i32 frames);
tc_i32 timeclocker_every_seconds(TimeClocker *clocker, tc_i32 seconds);
tc_i32 timeclocker_every_minutes(TimeClocker *clocker, tc_i32 minutes);

void timeclocker_condition_init(TimeClockerCondition *condition, tc_i32 kind, const char *name, tc_i32 value0, tc_i32 value1, tc_i32 value2);
tc_i32 timeclocker_condition_eval(TimeClocker *clocker, const TimeClockerCondition *condition);

void timeclocker_action_init(TimeClockerAction *action, tc_i32 kind, const char *name, tc_i32 value0, tc_i32 value1, tc_i32 value2);
void timeclocker_action_set_event(TimeClockerAction *action, tc_i32 event_id, const char *event_name);
tc_i32 timeclocker_action_run(TimeClocker *clocker, const TimeClockerAction *action);

void timeclocker_event_init(TimeClockerEvent *event_value);
tc_i32 timeclocker_alarm_pending(TimeClocker *clocker, const char *name);
tc_i32 timeclocker_poll_alarm(TimeClocker *clocker, TimeClockerEvent *event_value, tc_i32 consume);

#ifdef __cplusplus
}
#endif

#endif
