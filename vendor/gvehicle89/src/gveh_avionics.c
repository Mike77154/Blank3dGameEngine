#include "gveh_avionics.h"

void gveh_avionics_default(gveh_avionics_cfg *cfg)
{
    cfg->flags = 0;
    cfg->radar_range = gveh_fx_from_int(2000);
    cfg->lock_cone = GVEH_FX_ONE / 2;
    cfg->lock_ticks = 45;
    cfg->scan_ticks = 20;
    cfg->threat_ticks = 40;
}

void gveh_avionics_arcade(gveh_avionics_cfg *cfg)
{
    gveh_avionics_default(cfg);
    cfg->flags = GVEH_AVIONICS_FLAG_ENABLED | GVEH_AVIONICS_FLAG_RADAR | GVEH_AVIONICS_FLAG_RWR;
    cfg->lock_ticks = 22;
    cfg->scan_ticks = 12;
}

void gveh_avionics_sim(gveh_avionics_cfg *cfg)
{
    gveh_avionics_default(cfg);
    cfg->flags = GVEH_AVIONICS_FLAG_ENABLED | GVEH_AVIONICS_FLAG_RADAR | GVEH_AVIONICS_FLAG_TADS | GVEH_AVIONICS_FLAG_RWR;
    cfg->lock_ticks = 55;
    cfg->scan_ticks = 18;
}

void gveh_avionics_space(gveh_avionics_cfg *cfg)
{
    gveh_avionics_arcade(cfg);
    cfg->flags |= GVEH_AVIONICS_FLAG_SPACE;
    cfg->radar_range = gveh_fx_from_int(6000);
    cfg->lock_ticks = 18;
}

void gveh_avionics_clear(gveh_avionics_state *st)
{
    st->sensor_mode = GVEH_SENSOR_VISUAL;
    st->locked = 0;
    st->lock_timer = 0;
    st->scan_timer = 0;
    st->threat_warn = 0;
    st->target_id = -1;
    st->target_range = 0;
    st->target_aspect = 0;
}

void gveh_avionics_step(gveh_avionics_state *st, const gveh_avionics_cfg *cfg, const gveh_input *in, gveh_fx speed, gveh_i32 tick, gveh_fx_queue *fxq)
{
    if ((cfg->flags & GVEH_AVIONICS_FLAG_ENABLED) == 0u) return;
    st->scan_timer++;
    if (st->scan_timer >= cfg->scan_ticks) {
        st->scan_timer = 0;
        st->target_id = (gveh_i16)((tick / (cfg->scan_ticks + 1)) & 7);
        st->target_range = cfg->radar_range - gveh_fx_from_int((int)(tick & 127));
        if (st->target_range < gveh_fx_from_int(50)) st->target_range = gveh_fx_from_int(50);
        st->target_aspect = gveh_fx_div(speed, gveh_fx_from_int(160));
        st->target_aspect = gveh_fx_clamp(st->target_aspect, -GVEH_FX_ONE, GVEH_FX_ONE);
        if ((cfg->flags & GVEH_AVIONICS_FLAG_TADS) != 0u && speed < gveh_fx_from_int(35)) st->sensor_mode = GVEH_SENSOR_TADS;
        else if ((cfg->flags & GVEH_AVIONICS_FLAG_SPACE) != 0u) st->sensor_mode = GVEH_SENSOR_SPACE;
        else st->sensor_mode = GVEH_SENSOR_RADAR;
    }
    if (in->lock_on > 0 && st->target_id >= 0) {
        if (st->lock_timer < cfg->lock_ticks) st->lock_timer++;
    } else {
        if (st->lock_timer > 0) st->lock_timer--;
    }
    st->locked = (st->lock_timer >= cfg->lock_ticks) ? 1 : 0;
    if ((cfg->flags & GVEH_AVIONICS_FLAG_RWR) != 0u && ((tick / 90) & 1) != 0) st->threat_warn = cfg->threat_ticks;
    else if (st->threat_warn > 0) st->threat_warn--;
    if (st->locked != 0) gveh_fx_push(fxq, GVEH_FX_EVENT_ENGINE, 7, 0, 0, 0);
    if (st->threat_warn > 0 && (tick & 15) == 0) gveh_fx_push(fxq, GVEH_FX_EVENT_IMPACT, 4, 0, 0, 0);
}
