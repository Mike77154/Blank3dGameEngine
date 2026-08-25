#ifndef GVEH_AFTERTOUCH_H
#define GVEH_AFTERTOUCH_H

#include "gveh_body.h"

#define GVEH_AFTERTOUCH_ACTIVE 1u

typedef struct gveh_aftertouch_cfg_s {
    gveh_i16 max_ticks;
    gveh_fx force;
    gveh_fx yaw_force;
    gveh_fx min_crash;
} gveh_aftertouch_cfg;

typedef struct gveh_aftertouch_state_s {
    gveh_i16 ticks;
    gveh_u16 flags;
} gveh_aftertouch_state;

void gveh_aftertouch_default(gveh_aftertouch_cfg *cfg);
void gveh_aftertouch_clear(gveh_aftertouch_state *st);
void gveh_aftertouch_trigger(gveh_aftertouch_state *st, const gveh_aftertouch_cfg *cfg, gveh_fx impact);
void gveh_aftertouch_apply(gveh_aftertouch_state *st, const gveh_aftertouch_cfg *cfg, gveh_body *body, gveh_basis basis, gveh_fx control_x, gveh_fx control_z);

#endif
