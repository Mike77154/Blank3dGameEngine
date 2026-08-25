#include "rt_time.h"
#include <assert.h>

typedef struct fake_clock {
    rt_ticks ticks;
} fake_clock;

static rt_ticks fake_get_ticks(void *user)
{
    fake_clock *c;
    c = (fake_clock *)user;
    return c->ticks;
}

static void test_raw_sim_pause_scale(void)
{
    fake_clock clk;
    rt_time_source src;
    rt_time_context ctx;

    clk.ticks = 1000U;
    src.get_ticks = fake_get_ticks;
    src.ticks_per_second = 1000U;
    src.user = &clk;

    rt_time_init(&ctx, &src);
    assert(rt_time_is_valid(&ctx));

    clk.ticks += 100U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 100U);
    assert(rt_time_delta_ms(&ctx) == 100U);
    assert(rt_time_raw_now_ms(&ctx) == 100U);
    assert(rt_time_now_ms(&ctx) == 100U);

    rt_time_set_scale(&ctx, RT_TIME_FP_HALF);
    clk.ticks += 100U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 100U);
    assert(rt_time_delta_ms(&ctx) == 50U);
    assert(rt_time_now_ms(&ctx) == 150U);

    rt_time_set_paused(&ctx, 1);
    clk.ticks += 300U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 300U);
    assert(rt_time_delta_ms(&ctx) == 0U);
    assert(rt_time_raw_now_ms(&ctx) == 500U);
    assert(rt_time_now_ms(&ctx) == 150U);
}

static void test_clamp_and_backstep(void)
{
    fake_clock clk;
    rt_time_source src;
    rt_time_context ctx;

    clk.ticks = 1000U;
    src.get_ticks = fake_get_ticks;
    src.ticks_per_second = 1000U;
    src.user = &clk;
    rt_time_init(&ctx, &src);

    clk.ticks += 1000U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 1000U);
    assert(rt_time_delta_ms(&ctx) == RT_TIME_DEFAULT_MAX_STEP_MS);

    clk.ticks -= 50U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 0U);
    assert(rt_time_delta_ms(&ctx) == 0U);
    assert(rt_time_clock_backsteps(&ctx) == 1U);
}

static void test_wrap(void)
{
    fake_clock clk;
    rt_time_source src;
    rt_time_context ctx;

    clk.ticks = 4294967280U;
    src.get_ticks = fake_get_ticks;
    src.ticks_per_second = 1000U;
    src.user = &clk;
    rt_time_init(&ctx, &src);

    clk.ticks = 24U;
    rt_time_update(&ctx);
    assert(rt_time_raw_delta_ms(&ctx) == 40U);
    assert(rt_time_clock_backsteps(&ctx) == 0U);
}

static void test_timer_and_fixed_step(void)
{
    fake_clock clk;
    rt_time_source src;
    rt_time_context ctx;
    rt_timer timer;
    rt_fixed_step fs;
    int fires;
    int steps;

    clk.ticks = 0U;
    src.get_ticks = fake_get_ticks;
    src.ticks_per_second = 1000U;
    src.user = &clk;
    rt_time_init(&ctx, &src);
    rt_time_set_max_step_ms(&ctx, 0U);

    rt_timer_init(&timer);
    rt_timer_start_ms(&timer, 50U, 1);
    clk.ticks += 250U;
    rt_time_update(&ctx);
    fires = rt_timer_update_count(&timer, &ctx);
    assert(fires == 5);
    assert(rt_timer_remaining_ms(&timer) == 50U);

    rt_fixed_step_init_ms(&fs, 100U, 0);
    steps = rt_fixed_step_update(&fs, &ctx);
    assert(steps == 2);
    assert(rt_fixed_step_alpha(&fs) == RT_TIME_FP_HALF);

    clk.ticks += 100U;
    rt_time_update(&ctx);
    steps = rt_fixed_step_update(&fs, &ctx);
    assert(steps == 1);
    assert(rt_fixed_step_alpha(&fs) == RT_TIME_FP_HALF);
}

static void test_conversions(void)
{
    assert(rt_time_ms_to_q16_seconds(500U) == RT_TIME_FP_HALF);
    assert(rt_time_q16_seconds_to_ms(RT_TIME_FP_HALF) == 500U);
    assert(rt_time_ms_to_q16_seconds(1000U) == RT_TIME_FP_ONE);
}

int main(void)
{
    test_raw_sim_pause_scale();
    test_clamp_and_backstep();
    test_wrap();
    test_timer_and_fixed_step();
    test_conversions();
    return 0;
}
