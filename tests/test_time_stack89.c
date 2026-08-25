#include "blank3d_time89.h"
#include "blank3d_timeverbs89.h"
#include "gameverbs89.h"
#include <assert.h>
#include <string.h>

typedef struct FakeClock89 {
    rt_ticks ticks;
} FakeClock89;

static rt_ticks fake_ticks(void *user)
{
    FakeClock89 *c;
    c = (FakeClock89 *)user;
    return c->ticks;
}

static int alarm_hits;
static int alarm_a;
static int alarm_b;
static int alarm_c;

static int alarm_target(void *user, const gverb89_call *call)
{
    (void)user;
    assert(call != (const gverb89_call *)0);
    assert(call->argc == 3);
    alarm_a = call->argv[0][0] - '0';
    alarm_b = call->argv[1][0] - '0';
    alarm_c = call->argv[2][0] - '0';
    alarm_hits++;
    return GVERB89_HANDLED;
}

static void run_action(gverb89_registry *registry, const char *name,
                       const char **argv, int argc, long value_q16,
                       const char *value_text)
{
    gverb89_call call;
    memset(&call, 0, sizeof(call));
    call.name = name;
    call.argv = argv;
    call.argc = argc;
    call.value_q16 = value_q16;
    call.value_text = value_text ? value_text : "";
    call.has_value = argc > 0 || value_q16 != 0L ||
                     (value_text && value_text[0] != '\0');
    assert(gverb89_perform(registry, &call) == GVERB89_HANDLED);
}

static int run_condition(gverb89_registry *registry, const char *name,
                         const char *value_text)
{
    gverb89_call call;
    gverb89_result out;
    memset(&call, 0, sizeof(call));
    memset(&out, 0, sizeof(out));
    call.name = name;
    call.value_text = value_text ? value_text : "";
    call.has_value = value_text && value_text[0] != '\0';
    assert(gverb89_query(registry, &call, &out) == GVERB89_HANDLED);
    return out.truth;
}

int main(void)
{
    FakeClock89 fake;
    rt_time_source source;
    Blank3DTime89 time_value;
    Blank3DTimeVerbs89 verbs;
    gverb89_registry registry;
    const char *timer_args[3];

    fake.ticks = 1000U;
    source.get_ticks = fake_ticks;
    source.ticks_per_second = 1000U;
    source.user = &fake;

    assert(blank3d_time89_init(&time_value, &source, 50U, 60U));
    gverb89_init(&registry);
    assert(blank3d_timeverbs89_init(&verbs, &time_value, &registry));
    assert(gverb89_register_action(&registry, "alarm_target",
                                   alarm_target, (void *)0));

    fake.ticks += 100U;
    blank3d_time89_update(&time_value);
    assert(blank3d_time89_raw_delta_ms(&time_value) == 100U);
    assert(blank3d_time89_frame_ms(&time_value) == 50U);
    assert(tickoclock89_delta_ticks(&time_value.clock) == 3U);

    timer_args[0] = "door";
    timer_args[1] = "ms";
    timer_args[2] = "100";
    run_action(&registry, "timer_start", timer_args, 3, 0L, "");

    fake.ticks += 50U;
    blank3d_time89_update(&time_value);
    assert(!run_condition(&registry, "timer_done", "door"));
    fake.ticks += 50U;
    blank3d_time89_update(&time_value);
    assert(run_condition(&registry, "timer_done", "door"));

    {
        const char *alarm_args[7];
        alarm_args[0] = "wake";
        alarm_args[1] = "ms";
        alarm_args[2] = "50";
        alarm_args[3] = "alarm_target";
        alarm_args[4] = "7";
        alarm_args[5] = "8";
        alarm_args[6] = "9";
        run_action(&registry, "alarm_set", alarm_args, 7, 0L, "");
        fake.ticks += 50U;
        blank3d_time89_update(&time_value);
        assert(run_condition(&registry, "alarm_pending", "wake"));
        assert(blank3d_timeverbs89_pump_alarms(&verbs) == 1);
        assert(alarm_hits == 1);
        assert(alarm_a == 7 && alarm_b == 8 && alarm_c == 9);
        assert(!run_condition(&registry, "alarm_pending", "wake"));
    }

    run_action(&registry, "time_scale", (const char **)0, 0, 32768L, "0.5");
    fake.ticks += 100U;
    blank3d_time89_update(&time_value);
    assert(blank3d_time89_frame_ms(&time_value) == 25U);

    run_action(&registry, "time_pause", (const char **)0, 0, 65536L, "true");
    fake.ticks += 100U;
    blank3d_time89_update(&time_value);
    assert(blank3d_time89_frame_ms(&time_value) == 0U);
    assert(run_condition(&registry, "time_paused", ""));

    run_action(&registry, "time_resume", (const char **)0, 0, 65536L, "true");
    assert(!blank3d_time89_is_paused(&time_value));
    return 0;
}
