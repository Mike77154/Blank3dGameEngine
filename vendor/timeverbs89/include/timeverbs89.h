#ifndef TIMEVERBS89_H
#define TIMEVERBS89_H

/*
 * TimeVerbs89
 * Renderer/DSL-agnostic vocabulary for clocks, delta-time and named timers.
 * It resolves text into semantic enums; a host/provider owns the real clock.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TimeVerbs89Kind {
    TV89_KIND_NONE = 0,
    TV89_KIND_ACTION = 1,
    TV89_KIND_CONDITION = 2
} TimeVerbs89Kind;

typedef enum TimeVerbs89Unit {
    TV89_UNIT_NONE = 0,
    TV89_UNIT_TICKS = 1,
    TV89_UNIT_MILLISECONDS = 2,
    TV89_UNIT_FRAMES = 3,
    TV89_UNIT_SECONDS = 4,
    TV89_UNIT_MINUTES = 5,
    TV89_UNIT_HOURS = 6
} TimeVerbs89Unit;

typedef enum TimeVerbs89Verb {
    TV89_VERB_UNKNOWN = 0,

    TV89_ACT_TIME_PAUSE,
    TV89_ACT_TIME_RESUME,
    TV89_ACT_TIME_TOGGLE,
    TV89_ACT_TIME_SCALE,
    TV89_ACT_TIME_RESET,
    TV89_ACT_TIME_TICK_RATE,
    TV89_ACT_TIME_SET_HMS,

    TV89_ACT_TIMER_START,
    TV89_ACT_TIMER_ONCE,
    TV89_ACT_TIMER_LOOP,
    TV89_ACT_COOLDOWN_SET,
    TV89_ACT_STOPWATCH_START,
    TV89_ACT_FRAME_DELAY,
    TV89_ACT_TICK_DELAY,
    TV89_ACT_ALARM_SET,
    TV89_ACT_TIMER_STOP,
    TV89_ACT_TIMER_PAUSE,
    TV89_ACT_TIMER_RESUME,
    TV89_ACT_TIMER_RESET,
    TV89_ACT_TIMER_CLEAR,

    TV89_COND_TIME_PAUSED,
    TV89_COND_TIMER_EXISTS,
    TV89_COND_TIMER_ACTIVE,
    TV89_COND_TIMER_DONE,
    TV89_COND_TIMER_FIRED,
    TV89_COND_TIMER_READY,
    TV89_COND_COOLDOWN_READY,
    TV89_COND_ALARM_PENDING,
    TV89_COND_EVERY_TICK,
    TV89_COND_EVERY_FRAME,
    TV89_COND_EVERY_SECOND,
    TV89_COND_EVERY_TICKS,
    TV89_COND_EVERY_FRAMES,
    TV89_COND_EVERY_MS,
    TV89_COND_EVERY_SECONDS,
    TV89_COND_EVERY_MINUTES,
    TV89_COND_GLOBAL_TICKS_GTE,
    TV89_COND_GLOBAL_FRAMES_GTE,
    TV89_COND_GLOBAL_MS_GTE,
    TV89_COND_GLOBAL_SECONDS_GTE,
    TV89_COND_GLOBAL_MINUTES_GTE,
    TV89_COND_CLOCK_HOUR_EQ,
    TV89_COND_CLOCK_MINUTE_EQ,
    TV89_COND_CLOCK_SECOND_EQ
} TimeVerbs89Verb;

int tv89_resolve(const char *name, int *out_kind, int *out_verb);
int tv89_resolve_action(const char *name, int *out_verb);
int tv89_resolve_condition(const char *name, int *out_verb);
int tv89_unit_from_name(const char *name, int *out_unit);

int tv89_action_count(void);
const char *tv89_action_name(int ordinal);
int tv89_condition_count(void);
const char *tv89_condition_name(int ordinal);
const char *tv89_canonical_name(int verb);

#ifdef __cplusplus
}
#endif

#endif
