#include "gveh_tuning.h"
#include "gveh_tag.h"

static gveh_i16 gveh_stage_clamp(gveh_i16 v)
{
    if (v < 0) return 0;
    if (v > 5) return 5;
    return v;
}

void gveh_tuning_clear(gveh_tuning *t)
{
    t->engine_stage = 0;
    t->nitro_stage = 0;
    t->tire_stage = 0;
    t->brake_stage = 0;
    t->suspension_stage = 0;
    t->weight_stage = 0;
    t->aero_stage = 0;
    t->drift_stage = 0;
}

void gveh_tuning_street(gveh_tuning *t)
{
    gveh_tuning_clear(t);
    t->engine_stage = 2;
    t->nitro_stage = 2;
    t->tire_stage = 2;
    t->brake_stage = 1;
    t->suspension_stage = 1;
}

void gveh_tuning_drag(gveh_tuning *t)
{
    gveh_tuning_clear(t);
    t->engine_stage = 4;
    t->nitro_stage = 5;
    t->tire_stage = 2;
    t->brake_stage = 1;
    t->weight_stage = 2;
}

void gveh_tuning_drift(gveh_tuning *t)
{
    gveh_tuning_clear(t);
    t->engine_stage = 2;
    t->nitro_stage = 2;
    t->tire_stage = 1;
    t->brake_stage = 2;
    t->suspension_stage = 2;
    t->drift_stage = 4;
}

void gveh_tuning_apply(struct gveh_profile_s *p, const gveh_tuning *t)
{
    gveh_i16 eng;
    gveh_i16 nit;
    gveh_i16 tire;
    gveh_i16 brk;
    gveh_i16 susp;
    gveh_i16 wt;
    gveh_i16 drift;

    eng = gveh_stage_clamp(t->engine_stage);
    nit = gveh_stage_clamp(t->nitro_stage);
    tire = gveh_stage_clamp(t->tire_stage);
    brk = gveh_stage_clamp(t->brake_stage);
    susp = gveh_stage_clamp(t->suspension_stage);
    wt = gveh_stage_clamp(t->weight_stage);
    drift = gveh_stage_clamp(t->drift_stage);

    p->drive.drive_torque += gveh_fx_from_int(180 * eng);
    p->drive.brake_torque += gveh_fx_from_int(120 * brk);
    p->max_speed += gveh_fx_from_int(4 * eng);
    p->nitro.capacity += gveh_fx_from_int(20 * nit);
    p->nitro.force += gveh_fx_from_int(350 * nit);
    p->nitro.burn_rate += gveh_fx_from_int(2 * nit);
    p->susp.spring_k += GVEH_FX_ONE * susp;
    p->susp.damper_bump += GVEH_FX_ONE * susp;
    p->slide_power += (GVEH_FX_ONE / 2) * drift;
    p->tire.force_y[15] += (gveh_i16)(8 * tire);
    p->tire.force_x[15] += (gveh_i16)(6 * tire);
    p->mass -= gveh_fx_from_int(35 * wt);
    if (p->mass < gveh_fx_from_int(300)) p->mass = gveh_fx_from_int(300);
}
