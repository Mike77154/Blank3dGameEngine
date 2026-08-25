/* rt_time.h - protocol89 monotonic/simulation time core.
 *
 * C89, fixed-point/integer only, caller-owned storage, no heap.
 * The platform provides a wrapping 32-bit monotonic tick source.
 */
#ifndef RT_TIME_H
#define RT_TIME_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if INT_MAX < 2147483647
#error rt_time requires an at-least-32-bit signed int
#endif
#if UINT_MAX < 4294967295U
#error rt_time requires an at-least-32-bit unsigned int
#endif

typedef signed int rt_i32;
typedef unsigned int rt_u32;
typedef rt_u32 rt_ticks;
typedef rt_i32 rt_fixed;

#define RT_TIME_FP_SHIFT 16
#define RT_TIME_FP_ONE   ((rt_fixed)65536)
#define RT_TIME_FP_HALF  ((rt_fixed)32768)

#ifndef RT_TIME_DEFAULT_MAX_STEP_MS
#define RT_TIME_DEFAULT_MAX_STEP_MS 250U
#endif

#define RT_TIME_VERSION_MAJOR 2
#define RT_TIME_VERSION_MINOR 1
#define RT_TIME_VERSION_PATCH 0

typedef enum rt_time_status {
    RT_TIME_OK = 0,
    RT_TIME_ERR_NULL_CONTEXT,
    RT_TIME_ERR_NULL_SOURCE,
    RT_TIME_ERR_NULL_CALLBACK,
    RT_TIME_ERR_BAD_FREQUENCY,
    RT_TIME_ERR_NOT_INITIALIZED
} rt_time_status;

typedef struct rt_time_source {
    rt_ticks (*get_ticks)(void *user);
    rt_u32 ticks_per_second;
    void *user;
} rt_time_source;

typedef struct rt_time_config {
    rt_fixed scale_q16;
    rt_u32 max_step_ms;
    int paused;
    int clamp_enabled;
} rt_time_config;

typedef struct rt_time_context {
    rt_time_source src;
    rt_ticks start_ticks;
    rt_ticks last_ticks;

    rt_u32 raw_now_ms;
    rt_u32 raw_delta_ms;
    rt_u32 now_ms;
    rt_u32 delta_ms;

    rt_fixed scale_q16;
    rt_u32 max_step_ms;

    rt_u32 frame_count;
    rt_u32 clock_backsteps;

    int paused;
    int clamp_enabled;
    int initialized;

    rt_time_status last_status;
} rt_time_context;

void rt_time_config_default(rt_time_config *cfg);
rt_time_status rt_time_init_ex(rt_time_context *ctx,
                               const rt_time_source *src,
                               const rt_time_config *cfg);
void rt_time_init(rt_time_context *ctx, const rt_time_source *src);
void rt_time_reset(rt_time_context *ctx);
void rt_time_update(rt_time_context *ctx);

void rt_time_set_paused(rt_time_context *ctx, int paused);
void rt_time_toggle_paused(rt_time_context *ctx);
void rt_time_set_scale(rt_time_context *ctx, rt_fixed scale_q16);
void rt_time_set_max_step_ms(rt_time_context *ctx, rt_u32 max_step_ms);
void rt_time_set_clamp_enabled(rt_time_context *ctx, int enabled);

rt_u32 rt_time_raw_now_ms(const rt_time_context *ctx);
rt_u32 rt_time_raw_delta_ms(const rt_time_context *ctx);
rt_u32 rt_time_now_ms(const rt_time_context *ctx);
rt_u32 rt_time_delta_ms(const rt_time_context *ctx);
rt_fixed rt_time_raw_now_q16(const rt_time_context *ctx);
rt_fixed rt_time_raw_delta_q16(const rt_time_context *ctx);
rt_fixed rt_time_now_q16(const rt_time_context *ctx);
rt_fixed rt_time_delta_q16(const rt_time_context *ctx);
rt_u32 rt_time_frame_count(const rt_time_context *ctx);
rt_u32 rt_time_clock_backsteps(const rt_time_context *ctx);
int rt_time_is_paused(const rt_time_context *ctx);
int rt_time_is_valid(const rt_time_context *ctx);
rt_time_status rt_time_last_status(const rt_time_context *ctx);
const char *rt_time_status_string(rt_time_status status);

rt_u32 rt_time_ticks_to_ms(const rt_time_context *ctx, rt_ticks ticks);
rt_ticks rt_time_ms_to_ticks(const rt_time_context *ctx, rt_u32 ms);
rt_fixed rt_time_ms_to_q16_seconds(rt_u32 ms);
rt_u32 rt_time_q16_seconds_to_ms(rt_fixed seconds_q16);

typedef struct rt_timer {
    rt_u32 remaining_ms;
    rt_u32 period_ms;
    int active;
    int looping;
    int pending;
} rt_timer;

void rt_timer_init(rt_timer *t);
void rt_timer_start_ms(rt_timer *t, rt_u32 ms, int looping);
void rt_timer_start(rt_timer *t, const rt_time_context *ctx,
                    rt_fixed seconds_q16, int looping);
void rt_timer_stop(rt_timer *t);
int rt_timer_update(rt_timer *t, const rt_time_context *ctx);
int rt_timer_update_count(rt_timer *t, const rt_time_context *ctx);
rt_u32 rt_timer_remaining_ms(const rt_timer *t);
rt_fixed rt_timer_remaining(const rt_timer *t);
int rt_timer_is_active(const rt_timer *t);

typedef struct rt_fixed_step {
    rt_u32 step_ms;
    rt_u32 accumulator_ms;
    rt_fixed alpha_q16;
    int max_steps_per_update;
    rt_u32 total_steps;
    rt_u32 dropped_updates;
} rt_fixed_step;

void rt_fixed_step_init_ms(rt_fixed_step *fs, rt_u32 step_ms,
                           int max_steps_per_update);
void rt_fixed_step_init(rt_fixed_step *fs, rt_fixed step_seconds_q16,
                        int max_steps_per_update);
void rt_fixed_step_reset(rt_fixed_step *fs);
int rt_fixed_step_update(rt_fixed_step *fs, const rt_time_context *ctx);
rt_fixed rt_fixed_step_alpha(const rt_fixed_step *fs);
rt_u32 rt_fixed_step_total_steps(const rt_fixed_step *fs);
rt_u32 rt_fixed_step_dropped_updates(const rt_fixed_step *fs);

#ifdef __cplusplus
}
#endif

#endif
