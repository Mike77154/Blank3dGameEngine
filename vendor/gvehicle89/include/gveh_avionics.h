#ifndef GVEH_AVIONICS_H
#define GVEH_AVIONICS_H

#include "gveh_ai.h"
#include "gveh_fx.h"

#define GVEH_AVIONICS_FLAG_ENABLED     1u
#define GVEH_AVIONICS_FLAG_RADAR       2u
#define GVEH_AVIONICS_FLAG_TADS        4u
#define GVEH_AVIONICS_FLAG_RWR         8u
#define GVEH_AVIONICS_FLAG_SPACE       16u

#define GVEH_SENSOR_VISUAL 0
#define GVEH_SENSOR_RADAR  1
#define GVEH_SENSOR_TADS   2
#define GVEH_SENSOR_IRST   3
#define GVEH_SENSOR_SPACE  4

typedef struct gveh_avionics_cfg_s {
    gveh_u16 flags;
    gveh_fx radar_range;
    gveh_fx lock_cone;
    gveh_i16 lock_ticks;
    gveh_i16 scan_ticks;
    gveh_i16 threat_ticks;
} gveh_avionics_cfg;

typedef struct gveh_avionics_state_s {
    gveh_i16 sensor_mode;
    gveh_i16 locked;
    gveh_i16 lock_timer;
    gveh_i16 scan_timer;
    gveh_i16 threat_warn;
    gveh_i16 target_id;
    gveh_fx target_range;
    gveh_fx target_aspect;
} gveh_avionics_state;

void gveh_avionics_default(gveh_avionics_cfg *cfg);
void gveh_avionics_arcade(gveh_avionics_cfg *cfg);
void gveh_avionics_sim(gveh_avionics_cfg *cfg);
void gveh_avionics_space(gveh_avionics_cfg *cfg);
void gveh_avionics_clear(gveh_avionics_state *st);
void gveh_avionics_step(gveh_avionics_state *st, const gveh_avionics_cfg *cfg, const gveh_input *in, gveh_fx speed, gveh_i32 tick, gveh_fx_queue *fxq);

#endif
