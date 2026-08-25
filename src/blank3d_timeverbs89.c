#include "blank3d_timeverbs89.h"
#include "timeverbs89.h"
#include <limits.h>
#include <string.h>

#define B3D_TV89_MAX_ARGS 10
#define B3D_TV89_TEXT_CAP 192

typedef struct B3DTimeVerbArgs {
    char text[B3D_TV89_TEXT_CAP];
    const char *argv[B3D_TV89_MAX_ARGS];
    int argc;
} B3DTimeVerbArgs;

static int b3d_tv_is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
}

static void b3d_tv_args(B3DTimeVerbArgs *out,
                        const char **argv, int argc,
                        const char *value_text)
{
    int i;
    int n;
    char *p;
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (argv && argc > 0) {
        if (argc > B3D_TV89_MAX_ARGS) argc = B3D_TV89_MAX_ARGS;
        for (i = 0; i < argc; ++i) out->argv[i] = argv[i];
        out->argc = argc;
        return;
    }
    if (!value_text) return;
    for (i = 0; i + 1 < B3D_TV89_TEXT_CAP && value_text[i] != '\0'; ++i)
        out->text[i] = value_text[i];
    out->text[i] = '\0';

    p = out->text;
    n = 0;
    while (*p && n < B3D_TV89_MAX_ARGS) {
        while (*p && b3d_tv_is_space(*p)) {
            *p = '\0';
            ++p;
        }
        if (!*p) break;
        out->argv[n++] = p;
        if (*p == '"') {
            ++out->argv[n - 1];
            ++p;
            while (*p && *p != '"') ++p;
            if (*p == '"') {
                *p = '\0';
                ++p;
            }
        } else {
            while (*p && !b3d_tv_is_space(*p)) ++p;
        }
    }
    out->argc = n;
}

static int b3d_tv_parse_u32(const char *text, unsigned int *out)
{
    unsigned int value;
    unsigned int digit;
    if (!text || !out || text[0] == '\0') return 0;
    value = 0U;
    while (*text) {
        if (*text < '0' || *text > '9') return 0;
        digit = (unsigned int)(*text - '0');
        if (value > (UINT_MAX - digit) / 10U) value = UINT_MAX;
        else value = value * 10U + digit;
        ++text;
    }
    *out = value;
    return 1;
}

static int b3d_tv_parse_i32(const char *text, int *out)
{
    unsigned int value;
    int neg;
    if (!text || !out) return 0;
    neg = 0;
    if (*text == '-') {
        neg = 1;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    if (!b3d_tv_parse_u32(text, &value)) return 0;
    if (!neg) {
        *out = value > (unsigned int)INT_MAX ? INT_MAX : (int)value;
    } else {
        if (value > (unsigned int)INT_MAX) *out = INT_MIN + 1;
        else *out = -(int)value;
    }
    return 1;
}

static int b3d_tv_parse_q16(const char *text, int *out)
{
    int neg;
    unsigned int whole;
    unsigned int frac;
    unsigned int frac_scale;
    unsigned int q;
    unsigned int digit;
    if (!text || !out) return 0;
    neg = 0;
    whole = 0U;
    frac = 0U;
    frac_scale = 1U;
    if (*text == '-') { neg = 1; ++text; }
    else if (*text == '+') ++text;
    if ((*text < '0' || *text > '9') && *text != '.') return 0;
    while (*text >= '0' && *text <= '9') {
        digit = (unsigned int)(*text - '0');
        if (whole > 32767U) whole = 32767U;
        else whole = whole * 10U + digit;
        ++text;
    }
    if (*text == '.') {
        ++text;
        while (*text >= '0' && *text <= '9' && frac_scale < 1000000U) {
            digit = (unsigned int)(*text - '0');
            frac = frac * 10U + digit;
            frac_scale *= 10U;
            ++text;
        }
        while (*text >= '0' && *text <= '9') ++text;
    }
    if (*text != '\0') return 0;
    if (whole > 32767U) q = (unsigned int)INT_MAX;
    else {
        q = whole * 65536U;
        if (frac_scale > 1U)
            q += (frac * 65536U + frac_scale / 2U) / frac_scale;
        if (q > (unsigned int)INT_MAX) q = (unsigned int)INT_MAX;
    }
    *out = neg ? -(int)q : (int)q;
    return 1;
}

static int b3d_tv_unit(const char *name, int *unit)
{
    return tv89_unit_from_name(name, unit);
}

static unsigned int b3d_tv_period(const B3DTimeVerbArgs *a,
                                  long value_q16,
                                  int has_fallback)
{
    unsigned int v;
    if (a && a->argc > 0 && b3d_tv_parse_u32(a->argv[0], &v)) return v;
    if (has_fallback && value_q16 > 0L)
        return (unsigned int)(value_q16 >> 16);
    return 0U;
}

static int b3d_tv_action_impl(Blank3DTimeVerbs89 *bridge,
                              int verb,
                              long value_q16,
                              const char *value_text,
                              const char **argv,
                              int argc)
{
    B3DTimeVerbArgs a;
    Blank3DTime89 *t;
    unsigned int value;
    int unit;
    int repeat_limit;
    int scale_q16;
    int h;
    int m;
    int s;
    int ms;
    const char *name;

    if (!bridge || !bridge->time_value) return 0;
    t = bridge->time_value;
    b3d_tv_args(&a, argv, argc, value_text);

    switch (verb) {
        case TV89_ACT_TIME_PAUSE:
            blank3d_time89_pause(t); return 1;
        case TV89_ACT_TIME_RESUME:
            blank3d_time89_resume(t); return 1;
        case TV89_ACT_TIME_TOGGLE:
            blank3d_time89_toggle(t); return 1;
        case TV89_ACT_TIME_RESET:
            blank3d_time89_reset(t); return 1;
        case TV89_ACT_TIME_SCALE:
            if (value_q16 != 0L) scale_q16 = (int)value_q16;
            else if (a.argc > 0 && b3d_tv_parse_q16(a.argv[0], &scale_q16)) {
                /* parsed */
            } else return 0;
            blank3d_time89_set_scale_q16(t, scale_q16);
            return 1;
        case TV89_ACT_TIME_TICK_RATE:
            if (a.argc < 1 || !b3d_tv_parse_u32(a.argv[0], &value)) {
                if (value_q16 <= 0L) return 0;
                value = (unsigned int)(value_q16 >> 16);
            }
            blank3d_time89_set_tick_rate(t, value);
            return 1;
        case TV89_ACT_TIME_SET_HMS:
            if (a.argc < 3 ||
                !b3d_tv_parse_i32(a.argv[0], &h) ||
                !b3d_tv_parse_i32(a.argv[1], &m) ||
                !b3d_tv_parse_i32(a.argv[2], &s)) return 0;
            ms = 0;
            if (a.argc >= 4) (void)b3d_tv_parse_i32(a.argv[3], &ms);
            blank3d_time89_set_hms(t, h, m, s, ms);
            return 1;
        default: break;
    }

    if (a.argc < 1) return 0;
    name = a.argv[0];

    if (verb == TV89_ACT_TIMER_STOP)
        return blank3d_time89_timer_stop(t, name);
    if (verb == TV89_ACT_TIMER_PAUSE)
        return blank3d_time89_timer_pause(t, name);
    if (verb == TV89_ACT_TIMER_RESUME)
        return blank3d_time89_timer_resume(t, name);
    if (verb == TV89_ACT_TIMER_RESET)
        return blank3d_time89_timer_reset(t, name);
    if (verb == TV89_ACT_TIMER_CLEAR)
        return blank3d_time89_timer_clear(t, name);

    if (verb == TV89_ACT_STOPWATCH_START) {
        unit = TV89_UNIT_TICKS;
        if (a.argc >= 2 && !b3d_tv_unit(a.argv[1], &unit)) return 0;
        return blank3d_time89_stopwatch_start(t, name, unit);
    }
    if (verb == TV89_ACT_FRAME_DELAY || verb == TV89_ACT_TICK_DELAY) {
        if (a.argc < 2 || !b3d_tv_parse_u32(a.argv[1], &value)) return 0;
        unit = verb == TV89_ACT_FRAME_DELAY ? TV89_UNIT_FRAMES
                                            : TV89_UNIT_TICKS;
        return blank3d_time89_timer_once(t, name, unit, value);
    }

    if (a.argc < 3 || !b3d_tv_unit(a.argv[1], &unit) ||
        !b3d_tv_parse_u32(a.argv[2], &value)) return 0;

    if (verb == TV89_ACT_TIMER_START || verb == TV89_ACT_TIMER_ONCE)
        return blank3d_time89_timer_once(t, name, unit, value);

    if (verb == TV89_ACT_TIMER_LOOP) {
        repeat_limit = 0;
        if (a.argc >= 4) (void)b3d_tv_parse_i32(a.argv[3], &repeat_limit);
        return blank3d_time89_timer_loop(t, name, unit, value, repeat_limit);
    }
    if (verb == TV89_ACT_COOLDOWN_SET)
        return blank3d_time89_cooldown_set(t, name, unit, value);

    if (verb == TV89_ACT_ALARM_SET) {
        const char *action_name;
        int av;
        int bv;
        int cv;
        if (a.argc < 4) return 0;
        action_name = a.argv[3];
        av = bv = cv = 0;
        if (a.argc >= 5) (void)b3d_tv_parse_i32(a.argv[4], &av);
        if (a.argc >= 6) (void)b3d_tv_parse_i32(a.argv[5], &bv);
        if (a.argc >= 7) (void)b3d_tv_parse_i32(a.argv[6], &cv);
        return blank3d_time89_alarm_set(t, name, unit, value, 0,
                                        action_name, av, bv, cv);
    }
    return 0;
}

static int b3d_tv_condition_impl(Blank3DTimeVerbs89 *bridge,
                                 int verb,
                                 long value_q16,
                                 const char *value_text,
                                 const char **argv,
                                 int argc,
                                 int *out_truth)
{
    B3DTimeVerbArgs a;
    Blank3DTime89 *t;
    unsigned int value;
    TickOClock89HMS hms;
    const char *name;
    int truth;

    if (!out_truth) return 0;
    *out_truth = 0;
    if (!bridge || !bridge->time_value) return 0;
    t = bridge->time_value;
    b3d_tv_args(&a, argv, argc, value_text);
    truth = 0;

    if (verb == TV89_COND_TIME_PAUSED)
        truth = blank3d_time89_is_paused(t);
    else if (verb == TV89_COND_EVERY_TICK)
        truth = blank3d_time89_every(t, TV89_UNIT_TICKS, 1U);
    else if (verb == TV89_COND_EVERY_FRAME)
        truth = blank3d_time89_every(t, TV89_UNIT_FRAMES, 1U);
    else if (verb == TV89_COND_EVERY_SECOND)
        truth = blank3d_time89_every(t, TV89_UNIT_SECONDS, 1U);
    else if (verb == TV89_COND_EVERY_TICKS ||
             verb == TV89_COND_EVERY_FRAMES ||
             verb == TV89_COND_EVERY_MS ||
             verb == TV89_COND_EVERY_SECONDS ||
             verb == TV89_COND_EVERY_MINUTES) {
        value = b3d_tv_period(&a, value_q16, 1);
        if (value == 0U) value = 1U;
        if (verb == TV89_COND_EVERY_TICKS)
            truth = blank3d_time89_every(t, TV89_UNIT_TICKS, value);
        else if (verb == TV89_COND_EVERY_FRAMES)
            truth = blank3d_time89_every(t, TV89_UNIT_FRAMES, value);
        else if (verb == TV89_COND_EVERY_MS)
            truth = blank3d_time89_every(t, TV89_UNIT_MILLISECONDS, value);
        else if (verb == TV89_COND_EVERY_SECONDS)
            truth = blank3d_time89_every(t, TV89_UNIT_SECONDS, value);
        else truth = blank3d_time89_every(t, TV89_UNIT_MINUTES, value);
    } else if (verb == TV89_COND_GLOBAL_TICKS_GTE ||
               verb == TV89_COND_GLOBAL_FRAMES_GTE ||
               verb == TV89_COND_GLOBAL_MS_GTE ||
               verb == TV89_COND_GLOBAL_SECONDS_GTE ||
               verb == TV89_COND_GLOBAL_MINUTES_GTE) {
        int unit;
        value = b3d_tv_period(&a, value_q16, 1);
        unit = TV89_UNIT_TICKS;
        if (verb == TV89_COND_GLOBAL_FRAMES_GTE) unit = TV89_UNIT_FRAMES;
        else if (verb == TV89_COND_GLOBAL_MS_GTE)
            unit = TV89_UNIT_MILLISECONDS;
        else if (verb == TV89_COND_GLOBAL_SECONDS_GTE)
            unit = TV89_UNIT_SECONDS;
        else if (verb == TV89_COND_GLOBAL_MINUTES_GTE)
            unit = TV89_UNIT_MINUTES;
        truth = blank3d_time89_global(t, unit) >= value;
    } else if (verb == TV89_COND_CLOCK_HOUR_EQ ||
               verb == TV89_COND_CLOCK_MINUTE_EQ ||
               verb == TV89_COND_CLOCK_SECOND_EQ) {
        value = b3d_tv_period(&a, value_q16, 1);
        blank3d_time89_get_hms(t, &hms);
        if (verb == TV89_COND_CLOCK_HOUR_EQ)
            truth = hms.hours == (int)value;
        else if (verb == TV89_COND_CLOCK_MINUTE_EQ)
            truth = hms.minutes == (int)value;
        else truth = hms.seconds == (int)value;
    } else {
        if (a.argc < 1) return 1;
        name = a.argv[0];
        if (verb == TV89_COND_TIMER_EXISTS)
            truth = blank3d_time89_timer_exists(t, name);
        else if (verb == TV89_COND_TIMER_ACTIVE)
            truth = blank3d_time89_timer_active(t, name);
        else if (verb == TV89_COND_TIMER_DONE)
            truth = blank3d_time89_timer_done(t, name);
        else if (verb == TV89_COND_TIMER_FIRED)
            truth = blank3d_time89_timer_fired(t, name);
        else if (verb == TV89_COND_TIMER_READY)
            truth = blank3d_time89_timer_ready(t, name);
        else if (verb == TV89_COND_COOLDOWN_READY)
            truth = blank3d_time89_cooldown_ready(t, name);
        else if (verb == TV89_COND_ALARM_PENDING)
            truth = blank3d_time89_alarm_pending(t, name);
        else return 0;
    }

    *out_truth = truth != 0;
    return 1;
}

static int b3d_tv_game_action(void *user, const gverb89_call *call)
{
    Blank3DTimeVerbs89 *bridge;
    int verb;
    if (!user || !call) return GVERB89_ERROR;
    bridge = (Blank3DTimeVerbs89 *)user;
    if (!tv89_resolve_action(call->name, &verb)) return GVERB89_UNHANDLED;
    return b3d_tv_action_impl(bridge, verb, call->value_q16,
                              call->value_text, call->argv, call->argc)
        ? GVERB89_HANDLED : GVERB89_ERROR;
}

static int b3d_tv_game_condition(void *user, const gverb89_call *call,
                                 gverb89_result *out)
{
    Blank3DTimeVerbs89 *bridge;
    int verb;
    int truth;
    if (!user || !call || !out) return GVERB89_ERROR;
    bridge = (Blank3DTimeVerbs89 *)user;
    if (!tv89_resolve_condition(call->name, &verb))
        return GVERB89_UNHANDLED;
    if (!b3d_tv_condition_impl(bridge, verb, call->value_q16,
                               call->value_text, call->argv, call->argc,
                               &truth))
        return GVERB89_ERROR;
    out->truth = truth;
    out->value_q16 = truth ? 65536L : 0L;
    out->instance_id = 0;
    return GVERB89_HANDLED;
}

int blank3d_timeverbs89_init(Blank3DTimeVerbs89 *bridge,
                             Blank3DTime89 *time_value,
                             gverb89_registry *registry)
{
    int i;
    const char *name;
    if (!bridge || !time_value || !registry) return 0;
    memset(bridge, 0, sizeof(*bridge));
    bridge->time_value = time_value;
    bridge->registry = registry;

    for (i = 0; i < tv89_action_count(); ++i) {
        name = tv89_action_name(i);
        if (!name || !gverb89_register_action(registry, name,
                                               b3d_tv_game_action, bridge))
            return 0;
        bridge->registered_actions++;
    }
    for (i = 0; i < tv89_condition_count(); ++i) {
        name = tv89_condition_name(i);
        if (!name || !gverb89_register_condition(registry, name,
                                                  b3d_tv_game_condition,
                                                  bridge))
            return 0;
        bridge->registered_conditions++;
    }
    return 1;
}

int blank3d_timeverbs89_execute(Blank3DTimeVerbs89 *bridge,
                                const char *name,
                                long value_q16,
                                const char *value_text,
                                const char **argv,
                                int argc)
{
    int verb;
    if (!tv89_resolve_action(name, &verb)) return 0;
    return b3d_tv_action_impl(bridge, verb, value_q16,
                              value_text, argv, argc);
}

int blank3d_timeverbs89_query(Blank3DTimeVerbs89 *bridge,
                              const char *name,
                              long value_q16,
                              const char *value_text,
                              const char **argv,
                              int argc,
                              int *out_truth)
{
    int verb;
    if (!tv89_resolve_condition(name, &verb)) return 0;
    return b3d_tv_condition_impl(bridge, verb, value_q16,
                                 value_text, argv, argc, out_truth);
}


static void b3d_tv_i32_text(int value, char *out, int cap)
{
    unsigned int magnitude;
    char reverse[16];
    int n;
    int i;
    int pos;
    if (!out || cap <= 0) return;
    out[0] = '\0';
    if (value < 0) {
        magnitude = (unsigned int)(-(value + 1));
        magnitude += 1U;
    } else {
        magnitude = (unsigned int)value;
    }
    n = 0;
    do {
        reverse[n++] = (char)('0' + (magnitude % 10U));
        magnitude /= 10U;
    } while (magnitude != 0U && n < (int)sizeof(reverse));
    pos = 0;
    if (value < 0 && pos + 1 < cap) out[pos++] = '-';
    for (i = n - 1; i >= 0 && pos + 1 < cap; --i)
        out[pos++] = reverse[i];
    out[pos] = '\0';
}

static void b3d_tv_alarm_text(const char *a, const char *b, const char *c,
                              char *out, int cap)
{
    const char *parts[3];
    int part;
    int pos;
    int i;
    if (!out || cap <= 0) return;
    parts[0] = a ? a : "0";
    parts[1] = b ? b : "0";
    parts[2] = c ? c : "0";
    pos = 0;
    for (part = 0; part < 3; ++part) {
        if (part != 0 && pos + 1 < cap) out[pos++] = ' ';
        i = 0;
        while (parts[part][i] != '\0' && pos + 1 < cap)
            out[pos++] = parts[part][i++];
    }
    out[pos] = '\0';
}

static long b3d_tv_alarm_q16(int value)
{
    if (value > 32767) return 2147483647L;
    if (value < -32768) return (-2147483647L - 1L);
    return (long)value * 65536L;
}

int blank3d_timeverbs89_pump_alarms(Blank3DTimeVerbs89 *bridge)
{
    TimeClockerEvent event_value;
    gverb89_call call;
    const char *argv[3];
    char arg_text[3][16];
    char value_text[64];
    int dispatched;
    int result;
    int guard;
    int failed;

    if (!bridge || !bridge->time_value || !bridge->registry) return 0;
    dispatched = 0;
    failed = 0;
    guard = 0;
    while (guard < TIME_CLOCKER_MAX_TIMERS &&
           timeclocker_poll_alarm(&bridge->time_value->timers,
                                  &event_value, 1)) {
        ++guard;
        if (!event_value.valid || event_value.action_name[0] == '\0') {
            failed = 1;
            continue;
        }
        b3d_tv_i32_text(event_value.action_a, arg_text[0], 16);
        b3d_tv_i32_text(event_value.action_b, arg_text[1], 16);
        b3d_tv_i32_text(event_value.action_c, arg_text[2], 16);
        argv[0] = arg_text[0];
        argv[1] = arg_text[1];
        argv[2] = arg_text[2];
        b3d_tv_alarm_text(argv[0], argv[1], argv[2], value_text, 64);

        memset(&call, 0, sizeof(call));
        call.owner = (unsigned long)(unsigned int)event_value.action_id;
        call.name = event_value.action_name;
        call.value_q16 = b3d_tv_alarm_q16(event_value.action_a);
        call.value_text = value_text;
        call.has_value = 1;
        call.argv = argv;
        call.argc = 3;
        result = gverb89_perform(bridge->registry, &call);
        if (result == GVERB89_HANDLED) ++dispatched;
        else failed = 1;
    }
    return failed ? -1 : dispatched;
}
