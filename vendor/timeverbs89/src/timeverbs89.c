#include "timeverbs89.h"
#include <string.h>

typedef struct TV89NameMap {
    const char *name;
    int kind;
    int verb;
    int canonical;
} TV89NameMap;

static const TV89NameMap tv89_names[] = {
    { "time_pause", TV89_KIND_ACTION, TV89_ACT_TIME_PAUSE, 1 },
    { "pause_time", TV89_KIND_ACTION, TV89_ACT_TIME_PAUSE, 0 },
    { "time_resume", TV89_KIND_ACTION, TV89_ACT_TIME_RESUME, 1 },
    { "resume_time", TV89_KIND_ACTION, TV89_ACT_TIME_RESUME, 0 },
    { "time_toggle", TV89_KIND_ACTION, TV89_ACT_TIME_TOGGLE, 1 },
    { "time_scale", TV89_KIND_ACTION, TV89_ACT_TIME_SCALE, 1 },
    { "time_reset", TV89_KIND_ACTION, TV89_ACT_TIME_RESET, 1 },
    { "time_tick_rate", TV89_KIND_ACTION, TV89_ACT_TIME_TICK_RATE, 1 },
    { "tick_rate", TV89_KIND_ACTION, TV89_ACT_TIME_TICK_RATE, 0 },
    { "time_set_hms", TV89_KIND_ACTION, TV89_ACT_TIME_SET_HMS, 1 },
    { "clock_set_hms", TV89_KIND_ACTION, TV89_ACT_TIME_SET_HMS, 0 },

    { "timer_start", TV89_KIND_ACTION, TV89_ACT_TIMER_START, 1 },
    { "timer_once", TV89_KIND_ACTION, TV89_ACT_TIMER_ONCE, 1 },
    { "timer_loop", TV89_KIND_ACTION, TV89_ACT_TIMER_LOOP, 1 },
    { "cooldown_set", TV89_KIND_ACTION, TV89_ACT_COOLDOWN_SET, 1 },
    { "stopwatch_start", TV89_KIND_ACTION, TV89_ACT_STOPWATCH_START, 1 },
    { "frame_delay", TV89_KIND_ACTION, TV89_ACT_FRAME_DELAY, 1 },
    { "tick_delay", TV89_KIND_ACTION, TV89_ACT_TICK_DELAY, 1 },
    { "alarm_set", TV89_KIND_ACTION, TV89_ACT_ALARM_SET, 1 },
    { "timer_stop", TV89_KIND_ACTION, TV89_ACT_TIMER_STOP, 1 },
    { "timer_pause", TV89_KIND_ACTION, TV89_ACT_TIMER_PAUSE, 1 },
    { "timer_resume", TV89_KIND_ACTION, TV89_ACT_TIMER_RESUME, 1 },
    { "timer_reset", TV89_KIND_ACTION, TV89_ACT_TIMER_RESET, 1 },
    { "timer_clear", TV89_KIND_ACTION, TV89_ACT_TIMER_CLEAR, 1 },

    { "time_paused", TV89_KIND_CONDITION, TV89_COND_TIME_PAUSED, 1 },
    { "timer_exists", TV89_KIND_CONDITION, TV89_COND_TIMER_EXISTS, 1 },
    { "timer_active", TV89_KIND_CONDITION, TV89_COND_TIMER_ACTIVE, 1 },
    { "timer_done", TV89_KIND_CONDITION, TV89_COND_TIMER_DONE, 1 },
    { "timer_fired", TV89_KIND_CONDITION, TV89_COND_TIMER_FIRED, 1 },
    { "timer_ready", TV89_KIND_CONDITION, TV89_COND_TIMER_READY, 1 },
    { "cooldown_ready", TV89_KIND_CONDITION, TV89_COND_COOLDOWN_READY, 1 },
    { "alarm_pending", TV89_KIND_CONDITION, TV89_COND_ALARM_PENDING, 1 },
    { "every_tick", TV89_KIND_CONDITION, TV89_COND_EVERY_TICK, 1 },
    { "every_frame", TV89_KIND_CONDITION, TV89_COND_EVERY_FRAME, 1 },
    { "every_second", TV89_KIND_CONDITION, TV89_COND_EVERY_SECOND, 1 },
    { "every_ticks", TV89_KIND_CONDITION, TV89_COND_EVERY_TICKS, 1 },
    { "every_frames", TV89_KIND_CONDITION, TV89_COND_EVERY_FRAMES, 1 },
    { "every_ms", TV89_KIND_CONDITION, TV89_COND_EVERY_MS, 1 },
    { "every_seconds", TV89_KIND_CONDITION, TV89_COND_EVERY_SECONDS, 1 },
    { "every_minutes", TV89_KIND_CONDITION, TV89_COND_EVERY_MINUTES, 1 },
    { "global_ticks_gte", TV89_KIND_CONDITION, TV89_COND_GLOBAL_TICKS_GTE, 1 },
    { "global_frames_gte", TV89_KIND_CONDITION, TV89_COND_GLOBAL_FRAMES_GTE, 1 },
    { "global_ms_gte", TV89_KIND_CONDITION, TV89_COND_GLOBAL_MS_GTE, 1 },
    { "global_seconds_gte", TV89_KIND_CONDITION, TV89_COND_GLOBAL_SECONDS_GTE, 1 },
    { "global_minutes_gte", TV89_KIND_CONDITION, TV89_COND_GLOBAL_MINUTES_GTE, 1 },
    { "clock_hour_eq", TV89_KIND_CONDITION, TV89_COND_CLOCK_HOUR_EQ, 1 },
    { "clock_minute_eq", TV89_KIND_CONDITION, TV89_COND_CLOCK_MINUTE_EQ, 1 },
    { "clock_second_eq", TV89_KIND_CONDITION, TV89_COND_CLOCK_SECOND_EQ, 1 }
};

#define TV89_NAME_COUNT ((int)(sizeof(tv89_names) / sizeof(tv89_names[0])))

static int tv89_eq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

int tv89_resolve(const char *name, int *out_kind, int *out_verb)
{
    int i;
    if (!name || !out_kind || !out_verb) return 0;
    for (i = 0; i < TV89_NAME_COUNT; ++i) {
        if (tv89_eq(name, tv89_names[i].name)) {
            *out_kind = tv89_names[i].kind;
            *out_verb = tv89_names[i].verb;
            return 1;
        }
    }
    *out_kind = TV89_KIND_NONE;
    *out_verb = TV89_VERB_UNKNOWN;
    return 0;
}

int tv89_resolve_action(const char *name, int *out_verb)
{
    int kind;
    int verb;
    if (!out_verb || !tv89_resolve(name, &kind, &verb) ||
        kind != TV89_KIND_ACTION) return 0;
    *out_verb = verb;
    return 1;
}

int tv89_resolve_condition(const char *name, int *out_verb)
{
    int kind;
    int verb;
    if (!out_verb || !tv89_resolve(name, &kind, &verb) ||
        kind != TV89_KIND_CONDITION) return 0;
    *out_verb = verb;
    return 1;
}

int tv89_unit_from_name(const char *name, int *out_unit)
{
    int unit;
    if (!name || !out_unit) return 0;
    unit = TV89_UNIT_NONE;
    if (tv89_eq(name, "tick") || tv89_eq(name, "ticks"))
        unit = TV89_UNIT_TICKS;
    else if (tv89_eq(name, "ms") || tv89_eq(name, "millisecond") ||
             tv89_eq(name, "milliseconds"))
        unit = TV89_UNIT_MILLISECONDS;
    else if (tv89_eq(name, "frame") || tv89_eq(name, "frames") ||
             tv89_eq(name, "frametime"))
        unit = TV89_UNIT_FRAMES;
    else if (tv89_eq(name, "second") || tv89_eq(name, "seconds") ||
             tv89_eq(name, "sec"))
        unit = TV89_UNIT_SECONDS;
    else if (tv89_eq(name, "minute") || tv89_eq(name, "minutes") ||
             tv89_eq(name, "min"))
        unit = TV89_UNIT_MINUTES;
    else if (tv89_eq(name, "hour") || tv89_eq(name, "hours") ||
             tv89_eq(name, "hr"))
        unit = TV89_UNIT_HOURS;
    if (unit == TV89_UNIT_NONE) return 0;
    *out_unit = unit;
    return 1;
}

static int tv89_count_kind(int kind)
{
    int i;
    int count;
    count = 0;
    for (i = 0; i < TV89_NAME_COUNT; ++i)
        if (tv89_names[i].canonical && tv89_names[i].kind == kind) count++;
    return count;
}

static const char *tv89_name_kind_at(int kind, int ordinal)
{
    int i;
    int count;
    if (ordinal < 0) return (const char *)0;
    count = 0;
    for (i = 0; i < TV89_NAME_COUNT; ++i) {
        if (!tv89_names[i].canonical || tv89_names[i].kind != kind) continue;
        if (count == ordinal) return tv89_names[i].name;
        count++;
    }
    return (const char *)0;
}

int tv89_action_count(void)
{
    return tv89_count_kind(TV89_KIND_ACTION);
}
const char *tv89_action_name(int ordinal)
{
    return tv89_name_kind_at(TV89_KIND_ACTION, ordinal);
}
int tv89_condition_count(void)
{
    return tv89_count_kind(TV89_KIND_CONDITION);
}
const char *tv89_condition_name(int ordinal)
{
    return tv89_name_kind_at(TV89_KIND_CONDITION, ordinal);
}

const char *tv89_canonical_name(int verb)
{
    int i;
    for (i = 0; i < TV89_NAME_COUNT; ++i)
        if (tv89_names[i].canonical && tv89_names[i].verb == verb)
            return tv89_names[i].name;
    return "";
}
