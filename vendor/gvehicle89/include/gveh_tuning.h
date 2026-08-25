#ifndef GVEH_TUNING_H
#define GVEH_TUNING_H

#include "gveh_types.h"

struct gveh_profile_s;

typedef struct gveh_tuning_s {
    gveh_i16 engine_stage;
    gveh_i16 nitro_stage;
    gveh_i16 tire_stage;
    gveh_i16 brake_stage;
    gveh_i16 suspension_stage;
    gveh_i16 weight_stage;
    gveh_i16 aero_stage;
    gveh_i16 drift_stage;
} gveh_tuning;

void gveh_tuning_clear(gveh_tuning *t);
void gveh_tuning_street(gveh_tuning *t);
void gveh_tuning_drag(gveh_tuning *t);
void gveh_tuning_drift(gveh_tuning *t);
void gveh_tuning_apply(struct gveh_profile_s *p, const gveh_tuning *t);

#endif
