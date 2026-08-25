#ifndef BLANK3D_TIME89_H
#define BLANK3D_TIME89_H

#include "rt_time.h"
#include "timeclocker.h"
#include "tickoclock89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Blank3DTime89 {
    rt_time_context realtime;
    TimeClocker timers;
    TickOClock89 clock;
    unsigned int effective_frame_ms;
    int initialized;
    char status[128];
} Blank3DTime89;

int blank3d_time89_init(Blank3DTime89 *time_value,
                        const rt_time_source *source,
                        unsigned int max_step_ms,
                        unsigned int tick_rate);
void blank3d_time89_reset(Blank3DTime89 *time_value);
void blank3d_time89_update(Blank3DTime89 *time_value);

unsigned int blank3d_time89_frame_ms(const Blank3DTime89 *time_value);
rt_fixed blank3d_time89_delta_q16(const Blank3DTime89 *time_value);
rt_fixed blank3d_time89_now_q16(const Blank3DTime89 *time_value);
unsigned int blank3d_time89_now_ms(const Blank3DTime89 *time_value);
unsigned int blank3d_time89_raw_delta_ms(const Blank3DTime89 *time_value);

void blank3d_time89_pause(Blank3DTime89 *time_value);
void blank3d_time89_resume(Blank3DTime89 *time_value);
void blank3d_time89_toggle(Blank3DTime89 *time_value);
void blank3d_time89_set_scale_q16(Blank3DTime89 *time_value, int scale_q16);
void blank3d_time89_set_tick_rate(Blank3DTime89 *time_value,
                                  unsigned int tick_rate);
void blank3d_time89_set_hms(Blank3DTime89 *time_value,
                            int hour, int minute, int second, int millisecond);

int blank3d_time89_timer_once(Blank3DTime89 *time_value,
                              const char *name, int unit, unsigned int value);
int blank3d_time89_timer_loop(Blank3DTime89 *time_value,
                              const char *name, int unit, unsigned int value,
                              int repeat_limit);
int blank3d_time89_cooldown_set(Blank3DTime89 *time_value,
                                const char *name, int unit,
                                unsigned int value);
int blank3d_time89_stopwatch_start(Blank3DTime89 *time_value,
                                   const char *name, int unit);
int blank3d_time89_alarm_set(Blank3DTime89 *time_value,
                             const char *name, int unit, unsigned int value,
                             int action_id, const char *action_name,
                             int a, int b, int c);
int blank3d_time89_timer_stop(Blank3DTime89 *time_value, const char *name);
int blank3d_time89_timer_pause(Blank3DTime89 *time_value, const char *name);
int blank3d_time89_timer_resume(Blank3DTime89 *time_value, const char *name);
int blank3d_time89_timer_reset(Blank3DTime89 *time_value, const char *name);
int blank3d_time89_timer_clear(Blank3DTime89 *time_value, const char *name);

int blank3d_time89_timer_exists(const Blank3DTime89 *time_value,
                                const char *name);
int blank3d_time89_timer_active(const Blank3DTime89 *time_value,
                                const char *name);
int blank3d_time89_timer_done(const Blank3DTime89 *time_value,
                              const char *name);
int blank3d_time89_timer_fired(const Blank3DTime89 *time_value,
                               const char *name);
int blank3d_time89_timer_ready(const Blank3DTime89 *time_value,
                               const char *name);
int blank3d_time89_cooldown_ready(const Blank3DTime89 *time_value,
                                  const char *name);
int blank3d_time89_alarm_pending(const Blank3DTime89 *time_value,
                                 const char *name);

int blank3d_time89_every(const Blank3DTime89 *time_value,
                         int unit, unsigned int period);
unsigned int blank3d_time89_global(const Blank3DTime89 *time_value, int unit);
void blank3d_time89_get_hms(const Blank3DTime89 *time_value,
                            TickOClock89HMS *out_hms);
int blank3d_time89_is_paused(const Blank3DTime89 *time_value);

const char *blank3d_time89_status(const Blank3DTime89 *time_value);

#ifdef __cplusplus
}
#endif

#endif
